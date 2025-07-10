#include <glad/glad.h>
#include "VoxelBlockerDialog.h"
#include <QtWidgets/QApplication>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QDialog>

#include <QtGui/QMouseEvent>
#include <QtGui/QWheelEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QPainter>
#include <QtCore/QDir>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <queue>
#include <QtCore/QJsonArray>
#include <QtCore/QDebug>
#include <cmath>
#include <algorithm>
#include <set>
#include <map>

// Assimp includes for FBX export
#include <assimp/Exporter.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// VoxelCanvas Implementation
VoxelCanvas::VoxelCanvas(QWidget* parent)
    : QOpenGLWidget(parent)
    , m_cameraPosition(5.0f, 5.0f, 5.0f)
    , m_cameraTarget(0.0f, 0.0f, 0.0f)
    , m_cameraUp(0.0f, 1.0f, 0.0f)
    , m_cameraDistance(10.0f)
    , m_cameraYaw(45.0f)
    , m_cameraPitch(30.0f)
    , m_mousePressed(false)
    , m_pressedButton(Qt::NoButton)
    , m_currentFace(0)
    , m_isPanning(false)
    , m_isOrbiting(false)
    , m_selectedVoxel(nullptr)
    , m_selectedBone(nullptr)
    , m_selectedAnimation(nullptr)
    , m_nextBoneId(0)
    , m_nextAnimationId(0)
    , m_riggingMode(false)
    , m_showBones(true)
    , m_isPlaying(false)
    , m_currentAnimationTime(0.0f)
    , m_playingAnimationId(-1)
    , m_animationSpeed(1.0f)
    , m_hasKeyframeCopy(false)
    , m_currentTool(VoxelTool::Place)
    , m_symmetryMode(SymmetryMode::None)
    , m_symmetryCenter(0.0f)
    , m_isDragging(false)
    , m_dragStartPos(0.0f)
    , m_dragCurrentPos(0.0f)
    , m_gizmoMode(GizmoMode::Move)
    , m_hoveredGizmoAxis(GizmoAxis::None)
    , m_activeGizmoAxis(GizmoAxis::None)
    , m_gizmoInteracting(false)
    , m_gizmoStartPos(0.0f)
    , m_gizmoCurrentPos(0.0f)
    , m_undoIndex(0)
    , m_maxUndoSteps(100)
    , m_recordingUndoGroup(false)
    , m_currentUndoGroup(nullptr)
    , m_voxelSize(1.0f)
    , m_voxelColor(Qt::red)
    , m_placementMode(VoxelPlacementMode::GridSnap)
    , m_gridSize(1.0f)
    , m_gridBaseY(0.0f)
    , m_showGrid(true)
    , m_showPreview(false)
    , m_shaderProgram(0)
    , m_cubeVAO(0)
    , m_cubeVBO(0)
    , m_cubeEBO(0)
    , m_gridVAO(0)
    , m_gridVBO(0)
    , m_colorVBO(0)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    updateCamera();

    // Initialize animation timer
    m_animationTimer = new QTimer(this);
    connect(m_animationTimer, &QTimer::timeout, this, &VoxelCanvas::updateAnimation);
}

VoxelCanvas::~VoxelCanvas() {
    // Cleanup will be handled automatically
}

void VoxelCanvas::AddVoxel(const glm::vec3& position, const QColor& color, float size) {
    // Check if voxel already exists at this position
    for (const auto& voxel : m_voxels) {
        if (glm::distance(voxel.position, position) < 0.01f) {
            return; // Voxel already exists at this position
        }
    }

    Voxel voxel(position, color, size);
    m_voxels.push_back(voxel);

    update();
    emit voxelAdded(position);
    emit sceneModified();
}

void VoxelCanvas::RemoveVoxel(const glm::vec3& position) {
    for (auto it = m_voxels.begin(); it != m_voxels.end(); ++it) {
        if (glm::distance(it->position, position) < 0.01f) {
            m_voxels.erase(it);
            update();
            emit voxelRemoved(position);
            emit sceneModified();
            break;
        }
    }
}

void VoxelCanvas::ClearVoxels() {
    m_voxels.clear();
    m_selectedVoxel = nullptr;
    update();
    emit sceneModified();
}

void VoxelCanvas::SelectVoxel(const glm::vec3& position) {
    // Clear previous selection
    if (m_selectedVoxel) {
        m_selectedVoxel->selected = false;
    }

    // Find voxel at position
    for (auto& voxel : m_voxels) {
        if (glm::distance(voxel.position, position) < 0.01f) {
            voxel.selected = true;
            m_selectedVoxel = &voxel;
            emit voxelSelected(m_selectedVoxel);
            update();
            return;
        }
    }

    // No voxel found at position
    m_selectedVoxel = nullptr;
    emit voxelDeselected();
    update();
}

void VoxelCanvas::ClearSelection() {
    bool hadSelection = false;
    for (auto& voxel : m_voxels) {
        if (voxel.selected) {
            voxel.selected = false;
            hadSelection = true;
        }
    }

    if (m_selectedVoxel) {
        m_selectedVoxel = nullptr;
        hadSelection = true;
    }

    if (hadSelection) {
        emit voxelDeselected();
        update();
    }
}

std::vector<Voxel*> VoxelCanvas::GetSelectedVoxels() {
    std::vector<Voxel*> selected;
    for (auto& voxel : m_voxels) {
        if (voxel.selected) {
            selected.push_back(&voxel);
        }
    }
    return selected;
}

void VoxelCanvas::SelectAll() {
    for (auto& voxel : m_voxels) {
        voxel.selected = true;
    }

    if (!m_voxels.empty()) {
        m_selectedVoxel = &m_voxels[0];
        emit voxelSelected(m_selectedVoxel);
        update();
    }
}

void VoxelCanvas::SelectInBox(const glm::vec3& min, const glm::vec3& max) {
    bool hasSelection = false;
    for (auto& voxel : m_voxels) {
        if (voxel.position.x >= min.x && voxel.position.x <= max.x &&
            voxel.position.y >= min.y && voxel.position.y <= max.y &&
            voxel.position.z >= min.z && voxel.position.z <= max.z) {
            voxel.selected = true;
            if (!hasSelection) {
                m_selectedVoxel = &voxel;
                hasSelection = true;
            }
        }
    }

    if (hasSelection) {
        emit voxelSelected(m_selectedVoxel);
        update();
    }
}

void VoxelCanvas::RemoveFromSelection(const glm::vec3& position) {
    for (auto& voxel : m_voxels) {
        if (glm::distance(voxel.position, position) < 0.01f && voxel.selected) {
            voxel.selected = false;
            if (m_selectedVoxel == &voxel) {
                // Find another selected voxel to be primary
                m_selectedVoxel = nullptr;
                for (auto& v : m_voxels) {
                    if (v.selected) {
                        m_selectedVoxel = &v;
                        break;
                    }
                }
                if (!m_selectedVoxel) {
                    emit voxelDeselected();
                }
            }
            update();
            return;
        }
    }
}

void VoxelCanvas::MoveSelectedVoxels(const glm::vec3& offset) {
    BeginUndoGroup("Move Voxels");

    for (auto& voxel : m_voxels) {
        if (voxel.selected) {
            voxel.position += offset;
        }
    }

    EndUndoGroup();
    emit sceneModified();
    update();
}

void VoxelCanvas::DeleteSelectedVoxels() {
    BeginUndoGroup("Delete Voxels");

    auto it = m_voxels.begin();
    while (it != m_voxels.end()) {
        if (it->selected) {
            it = m_voxels.erase(it);
        } else {
            ++it;
        }
    }

    m_selectedVoxel = nullptr;
    EndUndoGroup();
    emit voxelDeselected();
    emit sceneModified();
    update();
}

void VoxelCanvas::MoveSelectedVoxel(const glm::vec3& newPosition) {
    if (m_selectedVoxel) {
        // Check if position is already occupied
        for (const auto& voxel : m_voxels) {
            if (&voxel != m_selectedVoxel && glm::distance(voxel.position, newPosition) < 0.01f) {
                return; // Position occupied
            }
        }

        m_selectedVoxel->position = newPosition;
        emit sceneModified();
        update();
    }
}

void VoxelCanvas::DeleteSelectedVoxel() {
    if (m_selectedVoxel) {
        RemoveVoxel(m_selectedVoxel->position);
        m_selectedVoxel = nullptr;
        emit voxelDeselected();
    }
}

// Multi-selection operations
void VoxelCanvas::AddToSelection(const glm::vec3& position) {
    for (auto& voxel : m_voxels) {
        if (glm::distance(voxel.position, position) < 0.01f) {
            if (!voxel.selected) {
                voxel.selected = true;
                m_selectedVoxel = &voxel; // Update primary selection
                emit voxelSelected(&voxel);
                update();
            }
            return;
        }
    }
}

void VoxelCanvas::InvertSelection() {
    for (auto& voxel : m_voxels) {
        voxel.selected = !voxel.selected;
    }

    // Update primary selected voxel
    m_selectedVoxel = nullptr;
    for (auto& voxel : m_voxels) {
        if (voxel.selected) {
            m_selectedVoxel = &voxel;
            break;
        }
    }

    if (m_selectedVoxel) {
        emit voxelSelected(m_selectedVoxel);
    } else {
        emit voxelDeselected();
    }
    update();
}

int VoxelCanvas::GetSelectedVoxelCount() const {
    int count = 0;
    for (const auto& voxel : m_voxels) {
        if (voxel.selected) {
            count++;
        }
    }
    return count;
}

void VoxelCanvas::SetSelectedVoxelsColor(const QColor& color) {
    for (auto& voxel : m_voxels) {
        if (voxel.selected) {
            voxel.color = color;
        }
    }
    emit sceneModified();
    update();
}

// Bone management methods
int VoxelCanvas::CreateBone(const QString& name, const glm::vec3& position, int parentId) {
    VoxelBone bone(m_nextBoneId, name);
    bone.position = position;
    bone.parentId = parentId;

    // Add to parent's children list if parent exists
    if (parentId >= 0) {
        VoxelBone* parent = GetBone(parentId);
        if (parent) {
            parent->childIds.push_back(m_nextBoneId);
        }
    }

    m_bones.push_back(bone);
    emit boneCreated(m_nextBoneId);
    emit sceneModified();
    update();

    return m_nextBoneId++;
}

void VoxelCanvas::DeleteBone(int boneId) {
    auto it = std::find_if(m_bones.begin(), m_bones.end(),
                          [boneId](const VoxelBone& bone) { return bone.id == boneId; });

    if (it != m_bones.end()) {
        // Remove from parent's children list
        if (it->parentId >= 0) {
            VoxelBone* parent = GetBone(it->parentId);
            if (parent) {
                auto childIt = std::find(parent->childIds.begin(), parent->childIds.end(), boneId);
                if (childIt != parent->childIds.end()) {
                    parent->childIds.erase(childIt);
                }
            }
        }

        // Reparent children to this bone's parent
        for (int childId : it->childIds) {
            VoxelBone* child = GetBone(childId);
            if (child) {
                child->parentId = it->parentId;
                if (it->parentId >= 0) {
                    VoxelBone* parent = GetBone(it->parentId);
                    if (parent) {
                        parent->childIds.push_back(childId);
                    }
                }
            }
        }

        // Unassign all voxels from this bone
        for (auto& voxel : m_voxels) {
            if (voxel.boneId == boneId) {
                voxel.boneId = -1;
            }
        }

        // Clear selection if this bone was selected
        if (m_selectedBone && m_selectedBone->id == boneId) {
            m_selectedBone = nullptr;
            emit boneDeselected();
        }

        m_bones.erase(it);
        emit boneDeleted(boneId);
        emit sceneModified();
        update();
    }
}

void VoxelCanvas::SelectBone(int boneId) {
    VoxelBone* bone = GetBone(boneId);
    if (bone) {
        m_selectedBone = bone;
        emit boneSelected(bone);
        update();
    }
}

void VoxelCanvas::ClearBoneSelection() {
    if (m_selectedBone) {
        m_selectedBone = nullptr;
        emit boneDeselected();
        update();
    }
}

VoxelBone* VoxelCanvas::GetBone(int boneId) {
    auto it = std::find_if(m_bones.begin(), m_bones.end(),
                          [boneId](const VoxelBone& bone) { return bone.id == boneId; });
    return (it != m_bones.end()) ? &(*it) : nullptr;
}

// Bone hierarchy methods
void VoxelCanvas::SetBoneParent(int boneId, int parentId) {
    VoxelBone* bone = GetBone(boneId);
    VoxelBone* parent = GetBone(parentId);

    if (!bone || (parentId >= 0 && !parent)) return;

    // Check for circular dependency
    if (parentId >= 0 && IsBoneAncestor(boneId, parentId)) {
        qWarning() << "Cannot set bone parent: would create circular dependency";
        return;
    }

    // Remove from old parent's children list
    if (bone->parentId >= 0) {
        VoxelBone* oldParent = GetBone(bone->parentId);
        if (oldParent) {
            auto it = std::find(oldParent->childIds.begin(), oldParent->childIds.end(), boneId);
            if (it != oldParent->childIds.end()) {
                oldParent->childIds.erase(it);
            }
        }
    }

    // Set new parent
    bone->parentId = parentId;

    // Add to new parent's children list
    if (parentId >= 0 && parent) {
        parent->childIds.push_back(boneId);
    }

    // Update transforms
    UpdateBoneHierarchy(boneId);
    emit sceneModified();
    update();
}

void VoxelCanvas::RemoveBoneParent(int boneId) {
    SetBoneParent(boneId, -1);
}

bool VoxelCanvas::IsBoneAncestor(int ancestorId, int descendantId) const {
    if (ancestorId == descendantId) return true;

    auto it = std::find_if(m_bones.begin(), m_bones.end(),
                          [descendantId](const VoxelBone& bone) { return bone.id == descendantId; });

    if (it == m_bones.end() || it->parentId < 0) return false;

    return IsBoneAncestor(ancestorId, it->parentId);
}

void VoxelCanvas::UpdateBoneHierarchy(int boneId) {
    VoxelBone* bone = GetBone(boneId);
    if (!bone) return;

    // Update this bone's transform based on parent
    if (bone->parentId >= 0) {
        VoxelBone* parent = GetBone(bone->parentId);
        if (parent) {
            // Apply parent transform to bone
            // This is a simplified version - in a full implementation,
            // you'd want to maintain local and world transforms
            bone->worldPosition = parent->worldPosition + bone->position;
            bone->worldRotation = parent->worldRotation * bone->rotation;
            bone->worldScale = parent->worldScale * bone->scale;
        }
    } else {
        // Root bone - world transform equals local transform
        bone->worldPosition = bone->position;
        bone->worldRotation = bone->rotation;
        bone->worldScale = bone->scale;
    }

    // Recursively update children
    for (int childId : bone->childIds) {
        UpdateBoneHierarchy(childId);
    }
}

std::vector<int> VoxelCanvas::GetBoneChildren(int boneId) const {
    auto it = std::find_if(m_bones.begin(), m_bones.end(),
                          [boneId](const VoxelBone& bone) { return bone.id == boneId; });

    if (it != m_bones.end()) {
        return it->childIds;
    }

    return std::vector<int>();
}

void VoxelCanvas::MoveBone(int boneId, const glm::vec3& newPosition) {
    VoxelBone* bone = GetBone(boneId);
    if (bone) {
        bone->position = newPosition;
        UpdateBoneHierarchy(boneId); // Update hierarchy instead of just transforms
        UpdateBoneTransforms();
        emit sceneModified();
        update();
    }
}

void VoxelCanvas::ScaleBone(int boneId, const glm::vec3& scale) {
    VoxelBone* bone = GetBone(boneId);
    if (bone) {
        bone->scale = scale;
        UpdateBoneHierarchy(boneId);
        UpdateBoneTransforms();
        emit sceneModified();
        update();
    }
}

void VoxelCanvas::RotateBone(int boneId, const glm::quat& rotation) {
    VoxelBone* bone = GetBone(boneId);
    if (bone) {
        bone->rotation = rotation;
        UpdateBoneHierarchy(boneId);
        UpdateBoneTransforms();
        emit sceneModified();
        update();
    }
}

void VoxelCanvas::AssignVoxelToBone(const glm::vec3& voxelPosition, int boneId) {
    for (auto& voxel : m_voxels) {
        if (glm::distance(voxel.position, voxelPosition) < 0.01f) {
            voxel.boneId = boneId;
            emit sceneModified();
            update();
            break;
        }
    }
}

void VoxelCanvas::UnassignVoxelFromBone(const glm::vec3& voxelPosition) {
    for (auto& voxel : m_voxels) {
        if (glm::distance(voxel.position, voxelPosition) < 0.01f) {
            voxel.boneId = -1;
            emit sceneModified();
            update();
            break;
        }
    }
}

void VoxelCanvas::UpdateBoneTransforms() {
    // Update all voxels assigned to bones based on bone transforms
    for (auto& voxel : m_voxels) {
        if (voxel.boneId >= 0) {
            VoxelBone* bone = GetBone(voxel.boneId);
            if (bone) {
                // Store original position if not already stored
                if (voxel.originalPosition == glm::vec3(0.0f)) {
                    voxel.originalPosition = voxel.position;
                }

                // Calculate full transform matrix from bone's rest pose to current pose
                glm::mat4 restTransform = glm::mat4(1.0f);
                restTransform = glm::translate(restTransform, bone->restPosition);
                restTransform = restTransform * glm::mat4_cast(bone->restRotation);
                restTransform = glm::scale(restTransform, bone->restScale);

                glm::mat4 currentTransform = glm::mat4(1.0f);
                currentTransform = glm::translate(currentTransform, bone->position);
                currentTransform = currentTransform * glm::mat4_cast(bone->rotation);
                currentTransform = glm::scale(currentTransform, bone->scale);

                // Calculate the relative transform from rest to current
                glm::mat4 relativeTransform = currentTransform * glm::inverse(restTransform);

                // Apply transform to voxel position relative to bone's rest position
                glm::vec3 relativePos = voxel.originalPosition - bone->restPosition;
                glm::vec4 transformedPos = relativeTransform * glm::vec4(relativePos, 1.0f);
                voxel.position = bone->restPosition + glm::vec3(transformedPos);
            }
        }
    }
    emit sceneModified();
    update();
}

void VoxelCanvas::SetBoneKeyframe(int boneId, float time) {
    VoxelBone* bone = GetBone(boneId);
    if (!bone || !m_selectedAnimation) return;

    // Find or create animation track for this bone
    BoneAnimationTrack* track = nullptr;
    for (auto& t : m_selectedAnimation->tracks) {
        if (t.boneId == boneId) {
            track = &t;
            break;
        }
    }

    if (!track) {
        // Create new track for this bone
        m_selectedAnimation->tracks.emplace_back(boneId);
        track = &m_selectedAnimation->tracks.back();
    }

    // Find or create keyframe for this bone at this time
    for (auto& keyframe : track->keyframes) {
        if (abs(keyframe.time - time) < 0.01f) {
            // Update existing keyframe
            keyframe.position = bone->position;
            keyframe.rotation = bone->rotation;
            keyframe.scale = bone->scale;
            emit sceneModified();
            return;
        }
    }

    // Create new keyframe
    BoneKeyframe newKeyframe(time, bone->position, bone->rotation, bone->scale);
    track->keyframes.push_back(newKeyframe);

    // Sort keyframes by time
    std::sort(track->keyframes.begin(), track->keyframes.end(),
              [](const BoneKeyframe& a, const BoneKeyframe& b) {
                  return a.time < b.time;
              });

    emit sceneModified();
}

void VoxelCanvas::DeleteBoneKeyframe(int boneId, float time) {
    if (!m_selectedAnimation) return;

    // Find animation track for this bone
    for (auto& track : m_selectedAnimation->tracks) {
        if (track.boneId == boneId) {
            auto it = std::remove_if(track.keyframes.begin(), track.keyframes.end(),
                                    [time](const BoneKeyframe& keyframe) {
                                        return abs(keyframe.time - time) < 0.01f;
                                    });

            if (it != track.keyframes.end()) {
                track.keyframes.erase(it, track.keyframes.end());
                emit sceneModified();
            }
            break;
        }
    }
}

// Animation management methods
int VoxelCanvas::CreateAnimation(const QString& name, float duration) {
    VoxelAnimation animation(name, duration);
    m_animations.push_back(animation);
    emit animationCreated(m_nextAnimationId);
    emit sceneModified();

    return m_nextAnimationId++;
}

void VoxelCanvas::DeleteAnimation(int animationId) {
    auto it = std::find_if(m_animations.begin(), m_animations.end(),
                          [this, animationId](const VoxelAnimation& anim) {
                              return animationId < m_animations.size() &&
                                     &anim == &m_animations[animationId];
                          });

    if (it != m_animations.end()) {
        // Stop animation if it's currently playing
        if (m_playingAnimationId == animationId) {
            StopAnimation();
        }

        // Clear selection if this animation was selected
        if (m_selectedAnimation == &(*it)) {
            m_selectedAnimation = nullptr;
        }

        m_animations.erase(it);
        emit animationDeleted(animationId);
        emit sceneModified();
    }
}

void VoxelCanvas::SelectAnimation(int animationId) {
    if (animationId >= 0 && animationId < m_animations.size()) {
        m_selectedAnimation = &m_animations[animationId];
        emit animationSelected(m_selectedAnimation);
    }
}

void VoxelCanvas::AddKeyframe(int animationId, int boneId, float time,
                             const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale) {
    if (animationId >= 0 && animationId < m_animations.size()) {
        VoxelAnimation& animation = m_animations[animationId];

        // Find or create track for this bone
        BoneAnimationTrack* track = nullptr;
        for (auto& t : animation.tracks) {
            if (t.boneId == boneId) {
                track = &t;
                break;
            }
        }

        if (!track) {
            animation.tracks.emplace_back(boneId);
            track = &animation.tracks.back();
        }

        // Add keyframe (sorted by time)
        BoneKeyframe keyframe(time, position, rotation, scale);
        auto insertPos = std::lower_bound(track->keyframes.begin(), track->keyframes.end(), keyframe,
                                         [](const BoneKeyframe& a, const BoneKeyframe& b) {
                                             return a.time < b.time;
                                         });
        track->keyframes.insert(insertPos, keyframe);

        emit sceneModified();
    }
}

void VoxelCanvas::RemoveKeyframe(int animationId, int boneId, float time) {
    if (animationId >= 0 && animationId < m_animations.size()) {
        VoxelAnimation& animation = m_animations[animationId];

        for (auto& track : animation.tracks) {
            if (track.boneId == boneId) {
                auto it = std::find_if(track.keyframes.begin(), track.keyframes.end(),
                                      [time](const BoneKeyframe& kf) {
                                          return std::abs(kf.time - time) < 0.001f;
                                      });
                if (it != track.keyframes.end()) {
                    track.keyframes.erase(it);
                    emit sceneModified();
                }
                break;
            }
        }
    }
}

void VoxelCanvas::PlayAnimation(int animationId) {
    if (animationId >= 0 && animationId < m_animations.size()) {
        m_playingAnimationId = animationId;
        m_currentAnimationTime = 0.0f;
        m_isPlaying = true;

        // Start animation timer (60 FPS)
        m_animationTimer->start(16);
        update();
    }
}

void VoxelCanvas::StopAnimation() {
    m_isPlaying = false;
    m_playingAnimationId = -1;
    m_currentAnimationTime = 0.0f;

    // Stop animation timer
    m_animationTimer->stop();

    // Reset bones to their rest positions
    for (auto& bone : m_bones) {
        bone.position = bone.restPosition;
        bone.rotation = bone.restRotation;
        bone.scale = bone.restScale;
    }
    update();
}

void VoxelCanvas::SetAnimationTime(float time) {
    m_currentAnimationTime = time;

    if (m_playingAnimationId >= 0 && m_playingAnimationId < m_animations.size()) {
        const VoxelAnimation& animation = m_animations[m_playingAnimationId];

        // Apply animation to bones at current time
        for (const auto& track : animation.tracks) {
            VoxelBone* bone = GetBone(track.boneId);
            if (!bone || track.keyframes.empty()) continue;

            // Find keyframes to interpolate between
            if (track.keyframes.size() == 1) {
                // Single keyframe - use it directly
                const BoneKeyframe& kf = track.keyframes[0];
                bone->position = kf.position;
                bone->rotation = kf.rotation;
                bone->scale = kf.scale;
            } else {
                // Find surrounding keyframes
                auto nextIt = std::lower_bound(track.keyframes.begin(), track.keyframes.end(), time,
                                              [](const BoneKeyframe& kf, float t) {
                                                  return kf.time < t;
                                              });

                if (nextIt == track.keyframes.begin()) {
                    // Before first keyframe
                    const BoneKeyframe& kf = track.keyframes[0];
                    bone->position = kf.position;
                    bone->rotation = kf.rotation;
                    bone->scale = kf.scale;
                } else if (nextIt == track.keyframes.end()) {
                    // After last keyframe
                    const BoneKeyframe& kf = track.keyframes.back();
                    bone->position = kf.position;
                    bone->rotation = kf.rotation;
                    bone->scale = kf.scale;
                } else {
                    // Interpolate between keyframes
                    auto prevIt = nextIt - 1;
                    const BoneKeyframe& prev = *prevIt;
                    const BoneKeyframe& next = *nextIt;

                    float t = (time - prev.time) / (next.time - prev.time);
                    t = std::clamp(t, 0.0f, 1.0f);

                    // Linear interpolation for position and scale
                    bone->position = glm::mix(prev.position, next.position, t);
                    bone->scale = glm::mix(prev.scale, next.scale, t);

                    // Spherical linear interpolation for rotation
                    bone->rotation = glm::slerp(prev.rotation, next.rotation, t);
                }
            }
        }

        // Update bone hierarchy for all root bones (bones without parents)
        for (const auto& bone : m_bones) {
            if (bone.parentId < 0) {
                UpdateBoneHierarchy(bone.id);
            }
        }

        // Update voxel positions based on bone transforms
        UpdateBoneTransforms();
        update();
    }
}

void VoxelCanvas::updateAnimation() {
    if (!m_isPlaying || m_playingAnimationId < 0 || m_playingAnimationId >= m_animations.size()) {
        return;
    }

    const VoxelAnimation& animation = m_animations[m_playingAnimationId];

    // Update animation time
    m_currentAnimationTime += (16.0f / 1000.0f) * m_animationSpeed; // 16ms per frame

    // Check if animation finished
    if (m_currentAnimationTime >= animation.duration) {
        if (animation.looping) {
            m_currentAnimationTime = fmod(m_currentAnimationTime, animation.duration);
        } else {
            m_currentAnimationTime = animation.duration;
            StopAnimation();
            return;
        }
    }

    // Apply animation at current time
    SetAnimationTime(m_currentAnimationTime);
}

// Additional animation methods
float VoxelCanvas::GetAnimationDuration(int animationId) const {
    if (animationId >= 0 && animationId < m_animations.size()) {
        return m_animations[animationId].duration;
    }
    return 0.0f;
}

void VoxelCanvas::SetAnimationLooping(int animationId, bool looping) {
    if (animationId >= 0 && animationId < m_animations.size()) {
        m_animations[animationId].looping = looping;
        emit sceneModified();
    }
}

bool VoxelCanvas::IsAnimationLooping(int animationId) const {
    if (animationId >= 0 && animationId < m_animations.size()) {
        return m_animations[animationId].looping;
    }
    return false;
}

void VoxelCanvas::RenameAnimation(int animationId, const QString& newName) {
    if (animationId >= 0 && animationId < m_animations.size()) {
        m_animations[animationId].name = newName;
        emit sceneModified();
    }
}

void VoxelCanvas::SetAnimationDuration(int animationId, float duration) {
    if (animationId >= 0 && animationId < m_animations.size()) {
        m_animations[animationId].duration = std::max(0.1f, duration);
        emit sceneModified();
    }
}

// Keyframe management
std::vector<float> VoxelCanvas::GetKeyframeTimes(int animationId, int boneId) const {
    std::vector<float> times;

    if (animationId >= 0 && animationId < m_animations.size()) {
        const VoxelAnimation& animation = m_animations[animationId];

        for (const auto& track : animation.tracks) {
            if (track.boneId == boneId) {
                for (const auto& keyframe : track.keyframes) {
                    times.push_back(keyframe.time);
                }
                break;
            }
        }
    }

    return times;
}

BoneKeyframe* VoxelCanvas::GetKeyframe(int animationId, int boneId, float time) {
    if (animationId >= 0 && animationId < m_animations.size()) {
        VoxelAnimation& animation = m_animations[animationId];

        for (auto& track : animation.tracks) {
            if (track.boneId == boneId) {
                for (auto& keyframe : track.keyframes) {
                    if (std::abs(keyframe.time - time) < 0.001f) {
                        return &keyframe;
                    }
                }
                break;
            }
        }
    }

    return nullptr;
}

void VoxelCanvas::UpdateKeyframe(int animationId, int boneId, float time, const BoneKeyframe& keyframe) {
    BoneKeyframe* existing = GetKeyframe(animationId, boneId, time);
    if (existing) {
        *existing = keyframe;
        existing->time = time; // Ensure time doesn't change
        emit sceneModified();
    }
}

void VoxelCanvas::MoveKeyframe(int animationId, int boneId, float oldTime, float newTime) {
    BoneKeyframe* keyframe = GetKeyframe(animationId, boneId, oldTime);
    if (keyframe) {
        BoneKeyframe temp = *keyframe;
        RemoveKeyframe(animationId, boneId, oldTime);
        AddKeyframe(animationId, boneId, newTime, temp.position, temp.rotation, temp.scale);
    }
}

void VoxelCanvas::CopyKeyframe(int animationId, int boneId, float time) {
    BoneKeyframe* keyframe = GetKeyframe(animationId, boneId, time);
    if (keyframe) {
        m_copiedKeyframe = *keyframe;
        m_hasKeyframeCopy = true;
    }
}

void VoxelCanvas::PasteKeyframe(int animationId, int boneId, float time) {
    if (m_hasKeyframeCopy) {
        AddKeyframe(animationId, boneId, time, m_copiedKeyframe.position,
                   m_copiedKeyframe.rotation, m_copiedKeyframe.scale);
    }
}

// Animation export/import
bool VoxelCanvas::ExportAnimationToJSON(int animationId, const QString& filepath) {
    if (animationId < 0 || animationId >= m_animations.size()) {
        return false;
    }

    const VoxelAnimation& animation = m_animations[animationId];

    QJsonObject animationObj;
    animationObj["name"] = animation.name;
    animationObj["duration"] = animation.duration;
    animationObj["looping"] = animation.looping;

    QJsonArray tracksArray;
    for (const auto& track : animation.tracks) {
        QJsonObject trackObj;
        trackObj["boneId"] = track.boneId;

        QJsonArray keyframesArray;
        for (const auto& keyframe : track.keyframes) {
            QJsonObject keyframeObj;
            keyframeObj["time"] = keyframe.time;

            QJsonArray posArray;
            posArray.append(keyframe.position.x);
            posArray.append(keyframe.position.y);
            posArray.append(keyframe.position.z);
            keyframeObj["position"] = posArray;

            QJsonArray rotArray;
            rotArray.append(keyframe.rotation.x);
            rotArray.append(keyframe.rotation.y);
            rotArray.append(keyframe.rotation.z);
            keyframeObj["rotation"] = rotArray;

            QJsonArray scaleArray;
            scaleArray.append(keyframe.scale.x);
            scaleArray.append(keyframe.scale.y);
            scaleArray.append(keyframe.scale.z);
            keyframeObj["scale"] = scaleArray;

            keyframesArray.append(keyframeObj);
        }

        trackObj["keyframes"] = keyframesArray;
        tracksArray.append(trackObj);
    }

    animationObj["tracks"] = tracksArray;

    QJsonDocument doc(animationObj);

    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    file.write(doc.toJson());
    return true;
}

bool VoxelCanvas::ImportAnimationFromJSON(const QString& filepath) {
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);

    if (error.error != QJsonParseError::NoError) {
        return false;
    }

    QJsonObject animationObj = doc.object();

    VoxelAnimation animation;
    animation.name = animationObj["name"].toString();
    animation.duration = animationObj["duration"].toDouble();
    animation.looping = animationObj["looping"].toBool();

    QJsonArray tracksArray = animationObj["tracks"].toArray();
    for (const auto& trackValue : tracksArray) {
        QJsonObject trackObj = trackValue.toObject();

        BoneAnimationTrack track;
        track.boneId = trackObj["boneId"].toInt();

        QJsonArray keyframesArray = trackObj["keyframes"].toArray();
        for (const auto& keyframeValue : keyframesArray) {
            QJsonObject keyframeObj = keyframeValue.toObject();

            BoneKeyframe keyframe;
            keyframe.time = keyframeObj["time"].toDouble();

            QJsonArray posArray = keyframeObj["position"].toArray();
            keyframe.position = glm::vec3(posArray[0].toDouble(), posArray[1].toDouble(), posArray[2].toDouble());

            QJsonArray rotArray = keyframeObj["rotation"].toArray();
            keyframe.rotation = glm::vec3(rotArray[0].toDouble(), rotArray[1].toDouble(), rotArray[2].toDouble());

            QJsonArray scaleArray = keyframeObj["scale"].toArray();
            keyframe.scale = glm::vec3(scaleArray[0].toDouble(), scaleArray[1].toDouble(), scaleArray[2].toDouble());

            track.keyframes.push_back(keyframe);
        }

        animation.tracks.push_back(track);
    }

    m_animations.push_back(animation);
    emit sceneModified();
    return true;
}

bool VoxelCanvas::ExportAllAnimationsToJSON(const QString& filepath) {
    QJsonObject rootObj;
    QJsonArray animationsArray;

    for (const auto& animation : m_animations) {
        QJsonObject animationObj;
        animationObj["name"] = animation.name;
        animationObj["duration"] = animation.duration;
        animationObj["looping"] = animation.looping;

        QJsonArray tracksArray;
        for (const auto& track : animation.tracks) {
            QJsonObject trackObj;
            trackObj["boneId"] = track.boneId;

            QJsonArray keyframesArray;
            for (const auto& keyframe : track.keyframes) {
                QJsonObject keyframeObj;
                keyframeObj["time"] = keyframe.time;

                QJsonArray posArray;
                posArray.append(keyframe.position.x);
                posArray.append(keyframe.position.y);
                posArray.append(keyframe.position.z);
                keyframeObj["position"] = posArray;

                QJsonArray rotArray;
                rotArray.append(keyframe.rotation.x);
                rotArray.append(keyframe.rotation.y);
                rotArray.append(keyframe.rotation.z);
                keyframeObj["rotation"] = rotArray;

                QJsonArray scaleArray;
                scaleArray.append(keyframe.scale.x);
                scaleArray.append(keyframe.scale.y);
                scaleArray.append(keyframe.scale.z);
                keyframeObj["scale"] = scaleArray;

                keyframesArray.append(keyframeObj);
            }

            trackObj["keyframes"] = keyframesArray;
            tracksArray.append(trackObj);
        }

        animationObj["tracks"] = tracksArray;
        animationsArray.append(animationObj);
    }

    rootObj["animations"] = animationsArray;
    rootObj["version"] = "1.0";

    QJsonDocument doc(rootObj);

    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    file.write(doc.toJson());
    return true;
}

// Advanced tool implementations
void VoxelCanvas::BrushPaint(const glm::vec3& center, const QColor& color) {
    std::vector<glm::vec3> positions = GetVoxelsInRadius(center, m_brushSettings.size, m_brushSettings.spherical);

    for (const glm::vec3& pos : positions) {
        // Apply randomization if enabled
        if (m_brushSettings.randomize && (rand() / float(RAND_MAX)) > m_brushSettings.randomStrength) {
            continue;
        }

        AddVoxel(pos, color, m_voxelSize);

        // Apply symmetry if enabled
        if (m_symmetryMode != SymmetryMode::None) {
            ApplySymmetry(pos, color, m_voxelSize);
        }
    }
}

void VoxelCanvas::BrushErase(const glm::vec3& center) {
    std::vector<glm::vec3> positions = GetVoxelsInRadius(center, m_brushSettings.size, m_brushSettings.spherical);

    for (const glm::vec3& pos : positions) {
        RemoveVoxel(pos);

        // Apply symmetry if enabled
        if (m_symmetryMode != SymmetryMode::None) {
            std::vector<glm::vec3> symPositions = GetSymmetryPositions(pos);
            for (const glm::vec3& symPos : symPositions) {
                RemoveVoxel(symPos);
            }
        }
    }
}

void VoxelCanvas::DrawLine(const glm::vec3& start, const glm::vec3& end, const QColor& color, float size) {
    glm::vec3 direction = end - start;
    float length = glm::length(direction);

    if (length < 0.01f) {
        AddVoxel(start, color, size);
        return;
    }

    direction = glm::normalize(direction);
    float step = size * 0.5f; // Step size based on voxel size

    for (float t = 0.0f; t <= length; t += step) {
        glm::vec3 pos = start + direction * t;
        glm::vec3 snapPos = getSnapPosition(pos);
        AddVoxel(snapPos, color, size);

        // Apply symmetry if enabled
        if (m_symmetryMode != SymmetryMode::None) {
            ApplySymmetry(snapPos, color, size);
        }
    }
}

void VoxelCanvas::DrawRectangle(const glm::vec3& corner1, const glm::vec3& corner2, const QColor& color, float size, bool filled) {
    glm::vec3 min = glm::min(corner1, corner2);
    glm::vec3 max = glm::max(corner1, corner2);

    for (float x = min.x; x <= max.x; x += size) {
        for (float y = min.y; y <= max.y; y += size) {
            for (float z = min.z; z <= max.z; z += size) {
                bool isEdge = (x == min.x || x == max.x) ||
                             (y == min.y || y == max.y) ||
                             (z == min.z || z == max.z);

                if (filled || isEdge) {
                    glm::vec3 pos(x, y, z);
                    glm::vec3 snapPos = getSnapPosition(pos);
                    AddVoxel(snapPos, color, size);

                    // Apply symmetry if enabled
                    if (m_symmetryMode != SymmetryMode::None) {
                        ApplySymmetry(snapPos, color, size);
                    }
                }
            }
        }
    }
}

void VoxelCanvas::DrawSphere(const glm::vec3& center, float radius, const QColor& color, float voxelSize, bool filled) {
    float radiusSquared = radius * radius;

    for (float x = center.x - radius; x <= center.x + radius; x += voxelSize) {
        for (float y = center.y - radius; y <= center.y + radius; y += voxelSize) {
            for (float z = center.z - radius; z <= center.z + radius; z += voxelSize) {
                glm::vec3 pos(x, y, z);
                glm::vec3 diff = pos - center;
                float distanceSquared = glm::dot(diff, diff);

                bool shouldPlace = filled ? (distanceSquared <= radiusSquared) :
                                          (distanceSquared <= radiusSquared &&
                                           distanceSquared >= (radius - voxelSize) * (radius - voxelSize));

                if (shouldPlace) {
                    glm::vec3 snapPos = getSnapPosition(pos);
                    AddVoxel(snapPos, color, voxelSize);

                    // Apply symmetry if enabled
                    if (m_symmetryMode != SymmetryMode::None) {
                        ApplySymmetry(snapPos, color, voxelSize);
                    }
                }
            }
        }
    }
}

void VoxelCanvas::FloodFill(const glm::vec3& startPos, const QColor& newColor) {
    // Find the voxel at start position
    Voxel* startVoxel = getVoxelAt(startPos);
    if (!startVoxel) return;

    QColor originalColor = startVoxel->color;
    if (originalColor == newColor) return; // Same color, nothing to do

    // Use a queue for flood fill algorithm
    std::queue<glm::vec3> queue;
    std::set<glm::vec3, std::function<bool(const glm::vec3&, const glm::vec3&)>> visited(
        [](const glm::vec3& a, const glm::vec3& b) {
            if (a.x != b.x) return a.x < b.x;
            if (a.y != b.y) return a.y < b.y;
            return a.z < b.z;
        });

    queue.push(startPos);
    visited.insert(startPos);

    // 6-directional neighbors (face-adjacent)
    std::vector<glm::vec3> directions = {
        {m_voxelSize, 0, 0}, {-m_voxelSize, 0, 0},
        {0, m_voxelSize, 0}, {0, -m_voxelSize, 0},
        {0, 0, m_voxelSize}, {0, 0, -m_voxelSize}
    };

    while (!queue.empty()) {
        glm::vec3 currentPos = queue.front();
        queue.pop();

        // Change color of current voxel
        Voxel* currentVoxel = getVoxelAt(currentPos);
        if (currentVoxel && currentVoxel->color == originalColor) {
            currentVoxel->color = newColor;

            // Check all neighbors
            for (const glm::vec3& dir : directions) {
                glm::vec3 neighborPos = currentPos + dir;

                if (visited.find(neighborPos) == visited.end()) {
                    Voxel* neighborVoxel = getVoxelAt(neighborPos);
                    if (neighborVoxel && neighborVoxel->color == originalColor) {
                        queue.push(neighborPos);
                        visited.insert(neighborPos);
                    }
                }
            }
        }
    }

    emit sceneModified();
    update();
}

// Copy/paste operations
void VoxelCanvas::CopySelection(const glm::vec3& min, const glm::vec3& max, bool includeBones, bool includeAnimations) {
    BeginUndoGroup("Copy Selection");

    m_clipboard.Clear();
    m_clipboard.centerPoint = (min + max) * 0.5f;
    m_clipboard.boundingBoxMin = min;
    m_clipboard.boundingBoxMax = max;

    // Copy voxels in selection
    for (const auto& voxel : m_voxels) {
        if (voxel.position.x >= min.x && voxel.position.x <= max.x &&
            voxel.position.y >= min.y && voxel.position.y <= max.y &&
            voxel.position.z >= min.z && voxel.position.z <= max.z) {

            Voxel clipboardVoxel = voxel;
            clipboardVoxel.position -= m_clipboard.centerPoint; // Store relative position
            m_clipboard.voxels.push_back(clipboardVoxel);
        }
    }

    // Copy bones if requested
    if (includeBones) {
        std::set<int> copiedBoneIds;

        // Find bones that have voxels assigned within the selection
        for (const auto& voxel : m_clipboard.voxels) {
            if (voxel.boneId >= 0) {
                copiedBoneIds.insert(voxel.boneId);
            }
        }

        // Copy bones and their hierarchy
        for (int boneId : copiedBoneIds) {
            VoxelBone* bone = GetBone(boneId);
            if (bone) {
                VoxelBone clipboardBone = *bone;
                clipboardBone.position -= m_clipboard.centerPoint; // Store relative position
                m_clipboard.bones.push_back(clipboardBone);
            }
        }

        m_clipboard.hasBones = !m_clipboard.bones.empty();
    }

    // Copy animations if requested
    if (includeAnimations && !m_clipboard.bones.empty()) {
        // Copy animations that affect the copied bones
        std::set<int> copiedBoneIds;
        for (const auto& bone : m_clipboard.bones) {
            copiedBoneIds.insert(bone.id);
        }

        for (const auto& animation : m_animations) {
            VoxelAnimation clipboardAnimation = animation;

            // Filter tracks to only include copied bones
            auto trackIt = clipboardAnimation.tracks.begin();
            while (trackIt != clipboardAnimation.tracks.end()) {
                if (copiedBoneIds.find(trackIt->boneId) == copiedBoneIds.end()) {
                    trackIt = clipboardAnimation.tracks.erase(trackIt);
                } else {
                    ++trackIt;
                }
            }

            // Only add animation if it has relevant tracks
            if (!clipboardAnimation.tracks.empty()) {
                m_clipboard.animations.push_back(clipboardAnimation);
            }
        }

        m_clipboard.hasAnimations = !m_clipboard.animations.empty();
    }

    EndUndoGroup();
}

void VoxelCanvas::CopyVoxels(const std::vector<glm::vec3>& positions, bool includeBones, bool includeAnimations) {
    if (positions.empty()) return;

    BeginUndoGroup("Copy Voxels");

    m_clipboard.Clear();

    // Calculate center point and bounding box
    glm::vec3 sum(0.0f);
    glm::vec3 minPos = positions[0];
    glm::vec3 maxPos = positions[0];

    for (const glm::vec3& pos : positions) {
        sum += pos;
        minPos = glm::min(minPos, pos);
        maxPos = glm::max(maxPos, pos);
    }

    m_clipboard.centerPoint = sum / float(positions.size());
    m_clipboard.boundingBoxMin = minPos;
    m_clipboard.boundingBoxMax = maxPos;

    // Copy voxels at specified positions
    std::set<int> copiedBoneIds;
    for (const glm::vec3& pos : positions) {
        Voxel* voxel = getVoxelAt(pos);
        if (voxel) {
            Voxel clipboardVoxel = *voxel;
            clipboardVoxel.position -= m_clipboard.centerPoint; // Store relative position
            m_clipboard.voxels.push_back(clipboardVoxel);

            if (voxel->boneId >= 0) {
                copiedBoneIds.insert(voxel->boneId);
            }
        }
    }

    // Copy bones if requested
    if (includeBones && !copiedBoneIds.empty()) {
        for (int boneId : copiedBoneIds) {
            VoxelBone* bone = GetBone(boneId);
            if (bone) {
                VoxelBone clipboardBone = *bone;
                clipboardBone.position -= m_clipboard.centerPoint;
                m_clipboard.bones.push_back(clipboardBone);
            }
        }
        m_clipboard.hasBones = true;
    }

    // Copy animations if requested
    if (includeAnimations && !copiedBoneIds.empty()) {
        for (const auto& animation : m_animations) {
            VoxelAnimation clipboardAnimation = animation;

            // Filter tracks to only include copied bones
            auto trackIt = clipboardAnimation.tracks.begin();
            while (trackIt != clipboardAnimation.tracks.end()) {
                if (copiedBoneIds.find(trackIt->boneId) == copiedBoneIds.end()) {
                    trackIt = clipboardAnimation.tracks.erase(trackIt);
                } else {
                    ++trackIt;
                }
            }

            if (!clipboardAnimation.tracks.empty()) {
                m_clipboard.animations.push_back(clipboardAnimation);
            }
        }
        m_clipboard.hasAnimations = !m_clipboard.animations.empty();
    }

    EndUndoGroup();
}

void VoxelCanvas::CopySelectedVoxels(bool includeBones, bool includeAnimations) {
    std::vector<glm::vec3> selectedPositions;

    for (const auto& voxel : m_voxels) {
        if (voxel.selected) {
            selectedPositions.push_back(voxel.position);
        }
    }

    if (!selectedPositions.empty()) {
        CopyVoxels(selectedPositions, includeBones, includeAnimations);
    }
}

void VoxelCanvas::CopyAll(bool includeBones, bool includeAnimations) {
    if (m_voxels.empty()) return;

    BeginUndoGroup("Copy All");

    m_clipboard.Clear();

    // Calculate bounding box
    glm::vec3 minPos = m_voxels[0].position;
    glm::vec3 maxPos = m_voxels[0].position;

    for (const auto& voxel : m_voxels) {
        minPos = glm::min(minPos, voxel.position);
        maxPos = glm::max(maxPos, voxel.position);
    }

    m_clipboard.centerPoint = (minPos + maxPos) * 0.5f;
    m_clipboard.boundingBoxMin = minPos;
    m_clipboard.boundingBoxMax = maxPos;

    // Copy all voxels
    for (const auto& voxel : m_voxels) {
        Voxel clipboardVoxel = voxel;
        clipboardVoxel.position -= m_clipboard.centerPoint;
        m_clipboard.voxels.push_back(clipboardVoxel);
    }

    // Copy all bones if requested
    if (includeBones) {
        for (const auto& bone : m_bones) {
            VoxelBone clipboardBone = bone;
            clipboardBone.position -= m_clipboard.centerPoint;
            m_clipboard.bones.push_back(clipboardBone);
        }
        m_clipboard.hasBones = !m_clipboard.bones.empty();
    }

    // Copy all animations if requested
    if (includeAnimations) {
        m_clipboard.animations = m_animations;
        m_clipboard.hasAnimations = !m_clipboard.animations.empty();
    }

    EndUndoGroup();
}

void VoxelCanvas::Paste(const glm::vec3& position, bool pasteBones, bool pasteAnimations) {
    if (m_clipboard.voxels.empty()) return;

    BeginUndoGroup("Paste");

    // Create bone ID mapping for pasted bones
    std::map<int, int> boneIdMapping;

    // Paste bones first if requested and available
    if (pasteBones && m_clipboard.hasBones) {
        for (const auto& clipboardBone : m_clipboard.bones) {
            glm::vec3 worldPos = position + clipboardBone.position;

            // Create new bone with new ID
            int newBoneId = CreateBone(clipboardBone.name + "_copy", worldPos, -1);
            boneIdMapping[clipboardBone.id] = newBoneId;

            // Copy other bone properties
            VoxelBone* newBone = GetBone(newBoneId);
            if (newBone) {
                newBone->rotation = clipboardBone.rotation;
                newBone->scale = clipboardBone.scale;
                newBone->debugColor = clipboardBone.debugColor;
                newBone->visible = clipboardBone.visible;
            }
        }

        // Fix bone hierarchy after all bones are created
        for (const auto& clipboardBone : m_clipboard.bones) {
            if (clipboardBone.parentId >= 0) {
                auto parentIt = boneIdMapping.find(clipboardBone.parentId);
                auto childIt = boneIdMapping.find(clipboardBone.id);

                if (parentIt != boneIdMapping.end() && childIt != boneIdMapping.end()) {
                    VoxelBone* childBone = GetBone(childIt->second);
                    VoxelBone* parentBone = GetBone(parentIt->second);

                    if (childBone && parentBone) {
                        childBone->parentId = parentIt->second;
                        parentBone->childIds.push_back(childIt->second);
                    }
                }
            }
        }
    }

    // Paste voxels
    for (const auto& clipboardVoxel : m_clipboard.voxels) {
        glm::vec3 worldPos = position + clipboardVoxel.position;
        glm::vec3 snapPos = getSnapPosition(worldPos);

        Voxel newVoxel(snapPos, clipboardVoxel.color, clipboardVoxel.size);

        // Map bone assignment if bones were pasted
        if (clipboardVoxel.boneId >= 0 && pasteBones) {
            auto boneIt = boneIdMapping.find(clipboardVoxel.boneId);
            if (boneIt != boneIdMapping.end()) {
                newVoxel.boneId = boneIt->second;
            }
        }

        m_voxels.push_back(newVoxel);

        // Apply symmetry if enabled
        if (m_symmetryMode != SymmetryMode::None) {
            ApplySymmetry(snapPos, clipboardVoxel.color, clipboardVoxel.size);
        }
    }

    // Paste animations if requested and available
    if (pasteAnimations && m_clipboard.hasAnimations && !boneIdMapping.empty()) {
        for (const auto& clipboardAnimation : m_clipboard.animations) {
            VoxelAnimation newAnimation = clipboardAnimation;
            newAnimation.name += "_copy";

            // Update bone IDs in animation tracks
            for (auto& track : newAnimation.tracks) {
                auto boneIt = boneIdMapping.find(track.boneId);
                if (boneIt != boneIdMapping.end()) {
                    track.boneId = boneIt->second;
                } else {
                    // Remove tracks for bones that weren't pasted
                    track.boneId = -1;
                }
            }

            // Remove invalid tracks
            auto trackIt = newAnimation.tracks.begin();
            while (trackIt != newAnimation.tracks.end()) {
                if (trackIt->boneId == -1) {
                    trackIt = newAnimation.tracks.erase(trackIt);
                } else {
                    ++trackIt;
                }
            }

            // Only add animation if it has valid tracks
            if (!newAnimation.tracks.empty()) {
                m_animations.push_back(newAnimation);
            }
        }
    }

    emit sceneModified();
    update();
    EndUndoGroup();
}

void VoxelCanvas::PasteWithOffset(const glm::vec3& offset, bool pasteBones, bool pasteAnimations) {
    glm::vec3 pastePosition = m_clipboard.centerPoint + offset;
    Paste(pastePosition, pasteBones, pasteAnimations);
}

// Utility functions
std::vector<glm::vec3> VoxelCanvas::GetVoxelsInRadius(const glm::vec3& center, float radius, bool spherical) {
    std::vector<glm::vec3> positions;

    for (float x = center.x - radius; x <= center.x + radius; x += m_voxelSize) {
        for (float y = center.y - radius; y <= center.y + radius; y += m_voxelSize) {
            for (float z = center.z - radius; z <= center.z + radius; z += m_voxelSize) {
                glm::vec3 pos(x, y, z);

                if (spherical) {
                    if (glm::distance(pos, center) <= radius) {
                        positions.push_back(getSnapPosition(pos));
                    }
                } else {
                    // Cubic brush
                    positions.push_back(getSnapPosition(pos));
                }
            }
        }
    }

    return positions;
}

std::vector<glm::vec3> VoxelCanvas::GetSymmetryPositions(const glm::vec3& position) {
    std::vector<glm::vec3> positions;

    if (m_symmetryMode == SymmetryMode::None) {
        return positions;
    }

    glm::vec3 offset = position - m_symmetryCenter;

    // Generate mirrored positions based on symmetry mode
    if (m_symmetryMode == SymmetryMode::X || m_symmetryMode == SymmetryMode::XY ||
        m_symmetryMode == SymmetryMode::XZ || m_symmetryMode == SymmetryMode::XYZ) {
        positions.push_back(m_symmetryCenter + glm::vec3(-offset.x, offset.y, offset.z));
    }

    if (m_symmetryMode == SymmetryMode::Y || m_symmetryMode == SymmetryMode::XY ||
        m_symmetryMode == SymmetryMode::YZ || m_symmetryMode == SymmetryMode::XYZ) {
        positions.push_back(m_symmetryCenter + glm::vec3(offset.x, -offset.y, offset.z));
    }

    if (m_symmetryMode == SymmetryMode::Z || m_symmetryMode == SymmetryMode::XZ ||
        m_symmetryMode == SymmetryMode::YZ || m_symmetryMode == SymmetryMode::XYZ) {
        positions.push_back(m_symmetryCenter + glm::vec3(offset.x, offset.y, -offset.z));
    }

    // Add diagonal symmetries for multi-axis modes
    if (m_symmetryMode == SymmetryMode::XY) {
        positions.push_back(m_symmetryCenter + glm::vec3(-offset.x, -offset.y, offset.z));
    }

    if (m_symmetryMode == SymmetryMode::XZ) {
        positions.push_back(m_symmetryCenter + glm::vec3(-offset.x, offset.y, -offset.z));
    }

    if (m_symmetryMode == SymmetryMode::YZ) {
        positions.push_back(m_symmetryCenter + glm::vec3(offset.x, -offset.y, -offset.z));
    }

    if (m_symmetryMode == SymmetryMode::XYZ) {
        positions.push_back(m_symmetryCenter + glm::vec3(-offset.x, -offset.y, offset.z));
        positions.push_back(m_symmetryCenter + glm::vec3(-offset.x, offset.y, -offset.z));
        positions.push_back(m_symmetryCenter + glm::vec3(offset.x, -offset.y, -offset.z));
        positions.push_back(m_symmetryCenter + glm::vec3(-offset.x, -offset.y, -offset.z));
    }

    return positions;
}

void VoxelCanvas::ApplySymmetry(const glm::vec3& position, const QColor& color, float size) {
    std::vector<glm::vec3> symPositions = GetSymmetryPositions(position);
    for (const glm::vec3& symPos : symPositions) {
        AddVoxel(symPos, color, size);
    }
}

// Undo/Redo system implementation
void VoxelCanvas::Undo() {
    if (!CanUndo()) return;

    m_undoIndex--;
    const UndoAction& action = m_undoStack[m_undoIndex];

    // Restore previous state
    switch (action.type) {
        case UndoActionType::AddVoxel:
        case UndoActionType::RemoveVoxel:
        case UndoActionType::ModifyVoxel:
        case UndoActionType::BulkOperation:
            m_voxels = action.voxelsBefore;
            break;

        case UndoActionType::AddBone:
        case UndoActionType::RemoveBone:
        case UndoActionType::ModifyBone:
            m_bones = action.bonesBefore;
            break;

        case UndoActionType::AddAnimation:
        case UndoActionType::RemoveAnimation:
        case UndoActionType::ModifyAnimation:
            m_animations = action.animationsBefore;
            break;
    }

    // Clear any selections that might be invalid
    m_selectedVoxel = nullptr;
    m_selectedBone = nullptr;
    m_selectedAnimation = nullptr;

    emit sceneModified();
    update();
}

void VoxelCanvas::Redo() {
    if (!CanRedo()) return;

    const UndoAction& action = m_undoStack[m_undoIndex];
    m_undoIndex++;

    // Apply the action again
    switch (action.type) {
        case UndoActionType::AddVoxel:
        case UndoActionType::RemoveVoxel:
        case UndoActionType::ModifyVoxel:
        case UndoActionType::BulkOperation:
            m_voxels = action.voxelsAfter;
            break;

        case UndoActionType::AddBone:
        case UndoActionType::RemoveBone:
        case UndoActionType::ModifyBone:
            m_bones = action.bonesAfter;
            break;

        case UndoActionType::AddAnimation:
        case UndoActionType::RemoveAnimation:
        case UndoActionType::ModifyAnimation:
            m_animations = action.animationsAfter;
            break;
    }

    // Clear any selections that might be invalid
    m_selectedVoxel = nullptr;
    m_selectedBone = nullptr;
    m_selectedAnimation = nullptr;

    emit sceneModified();
    update();
}

void VoxelCanvas::ClearUndoStack() {
    m_undoStack.clear();
    m_undoIndex = 0;
    m_recordingUndoGroup = false;
    m_currentUndoGroup = nullptr;
}

void VoxelCanvas::BeginUndoGroup(const QString& description) {
    if (m_recordingUndoGroup) {
        EndUndoGroup(); // End previous group if still recording
    }

    m_recordingUndoGroup = true;

    // Remove any redo actions
    if (m_undoIndex < m_undoStack.size()) {
        m_undoStack.erase(m_undoStack.begin() + m_undoIndex, m_undoStack.end());
    }

    // Add new undo group
    m_undoStack.emplace_back(UndoActionType::BulkOperation, description);
    m_currentUndoGroup = &m_undoStack.back();

    // Capture current state as "before"
    m_currentUndoGroup->voxelsBefore = m_voxels;
    m_currentUndoGroup->bonesBefore = m_bones;
    m_currentUndoGroup->animationsBefore = m_animations;
}

void VoxelCanvas::EndUndoGroup() {
    if (!m_recordingUndoGroup || !m_currentUndoGroup) return;

    // Capture current state as "after"
    m_currentUndoGroup->voxelsAfter = m_voxels;
    m_currentUndoGroup->bonesAfter = m_bones;
    m_currentUndoGroup->animationsAfter = m_animations;

    // Check if anything actually changed
    bool hasChanges = (m_currentUndoGroup->voxelsBefore != m_currentUndoGroup->voxelsAfter) ||
                     (m_currentUndoGroup->bonesBefore != m_currentUndoGroup->bonesAfter) ||
                     (m_currentUndoGroup->animationsBefore != m_currentUndoGroup->animationsAfter);

    if (!hasChanges) {
        // No changes, remove the empty undo action
        m_undoStack.pop_back();
    } else {
        // Move to next undo index
        m_undoIndex = m_undoStack.size();

        // Limit undo stack size
        if (m_undoStack.size() > m_maxUndoSteps) {
            m_undoStack.erase(m_undoStack.begin());
            m_undoIndex--;
        }
    }

    m_recordingUndoGroup = false;
    m_currentUndoGroup = nullptr;
}

QString VoxelCanvas::GetUndoDescription() const {
    if (!CanUndo()) return QString();
    return m_undoStack[m_undoIndex - 1].description;
}

QString VoxelCanvas::GetRedoDescription() const {
    if (!CanRedo()) return QString();
    return m_undoStack[m_undoIndex].description;
}

// Helper method to record undo actions
void VoxelCanvas::RecordUndoAction(UndoActionType type, const QString& description) {
    if (m_recordingUndoGroup) return; // Don't record individual actions during group recording

    // Remove any redo actions
    if (m_undoIndex < m_undoStack.size()) {
        m_undoStack.erase(m_undoStack.begin() + m_undoIndex, m_undoStack.end());
    }

    // Create new undo action
    UndoAction action(type, description);

    // The "before" state should be captured before the operation
    // The "after" state will be captured after the operation
    // For now, we'll use the current state as "after"
    action.voxelsAfter = m_voxels;
    action.bonesAfter = m_bones;
    action.animationsAfter = m_animations;

    m_undoStack.push_back(action);
    m_undoIndex = m_undoStack.size();

    // Limit undo stack size
    if (m_undoStack.size() > m_maxUndoSteps) {
        m_undoStack.erase(m_undoStack.begin());
        m_undoIndex--;
    }
}

void VoxelCanvas::ResetCamera() {
    m_cameraYaw = 45.0f;
    m_cameraPitch = 30.0f;
    m_cameraDistance = 10.0f;
    updateCamera();
    update();
}

void VoxelCanvas::FocusOnVoxels() {
    if (m_voxels.empty()) {
        ResetCamera();
        return;
    }

    // Calculate bounding box
    glm::vec3 minPos = m_voxels[0].position;
    glm::vec3 maxPos = m_voxels[0].position;

    for (const auto& voxel : m_voxels) {
        minPos.x = std::min(minPos.x, voxel.position.x);
        minPos.y = std::min(minPos.y, voxel.position.y);
        minPos.z = std::min(minPos.z, voxel.position.z);
        maxPos.x = std::max(maxPos.x, voxel.position.x);
        maxPos.y = std::max(maxPos.y, voxel.position.y);
        maxPos.z = std::max(maxPos.z, voxel.position.z);
    }

    // Set camera target to center of bounding box
    m_cameraTarget = (minPos + maxPos) * 0.5f;

    // Adjust distance based on bounding box size
    glm::vec3 size = maxPos - minPos;
    float maxSize = std::max({size.x, size.y, size.z});
    m_cameraDistance = maxSize * 2.0f + 5.0f;

    updateCamera();
    update();
}

void VoxelCanvas::NewScene() {
    ClearVoxels();
    ResetCamera();
}

void VoxelCanvas::initializeGL() {
    try {
        initializeOpenGLFunctions();

        // Initialize GLAD
        if (!gladLoadGL()) {
            qCritical("Failed to initialize GLAD in VoxelCanvas");
            return;
        }

        // Check OpenGL version
        const char* version = (const char*)glGetString(GL_VERSION);
        if (!version) {
            qCritical("Failed to get OpenGL version in VoxelCanvas");
            return;
        }
        qDebug() << "VoxelCanvas OpenGL version:" << version;

        glEnable(GL_DEPTH_TEST);
        // Disable face culling for voxels so we can see all faces
        glDisable(GL_CULL_FACE);
        glClearColor(0.12f, 0.12f, 0.14f, 1.0f); // Match SceneViewPanel background

        // Check for OpenGL errors before setup
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            qCritical() << "OpenGL error before setup in VoxelCanvas:" << error;
            return;
        }

        setupShaders();
        setupBuffers();

        // Final error check
        error = glGetError();
        if (error != GL_NO_ERROR) {
            qCritical() << "OpenGL error after setup in VoxelCanvas:" << error;
        }
    }
    catch (const std::exception& e) {
        qCritical() << "Exception in VoxelCanvas::initializeGL:" << e.what();
    }
    catch (...) {
        qCritical() << "Unknown exception in VoxelCanvas::initializeGL";
    }
}

void VoxelCanvas::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);

    float aspect = float(w) / float(h ? h : 1);
    m_projectionMatrix = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);
}

void VoxelCanvas::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_showGrid) {
        drawGrid();
    }

    drawVoxels();

    if (m_showBones && !m_bones.empty()) {
        drawBones();
    }

    if (m_showPreview) {
        drawPreviewVoxel();
    }
}

void VoxelCanvas::mousePressEvent(QMouseEvent* event) {
    m_mousePressed = true;
    m_lastMousePos = event->pos();
    m_pressedButton = event->button();

    if (event->button() == Qt::LeftButton) {
        // Check for gizmo interaction first
        if (m_currentTool == VoxelTool::Select && !GetSelectedVoxels().empty()) {
            GizmoAxis hoveredAxis = GetHoveredGizmoAxis(event->pos());
            if (hoveredAxis != GizmoAxis::None) {
                StartGizmoInteraction(hoveredAxis, event->pos());
                return;
            }
        }

        glm::vec3 worldPos = screenToWorld(event->pos());
        glm::vec3 snapPos = getSnapPosition(worldPos);

        // Handle different tools
        switch (m_currentTool) {
            case VoxelTool::Place:
                if (event->modifiers() & Qt::ControlModifier) {
                    // Ctrl+Click: Select voxel even in place mode
                    SelectVoxel(snapPos);
                } else {
                    // Check if clicking on existing voxel for selection
                    Voxel* clickedVoxel = getVoxelAt(snapPos);
                    if (clickedVoxel) {
                        SelectVoxel(snapPos);
                    } else {
                        // Place new voxel with symmetry
                        ClearSelection();
                        std::vector<glm::vec3> positions = getSymmetryPositions(snapPos);
                        for (const auto& pos : positions) {
                            AddVoxel(pos, m_voxelColor, m_voxelSize);
                        }
                    }
                }
                break;

            case VoxelTool::Erase:
                {
                    // Erase voxel with symmetry
                    std::vector<glm::vec3> positions = getSymmetryPositions(snapPos);
                    for (const auto& pos : positions) {
                        RemoveVoxel(pos);
                    }
                }
                break;

            case VoxelTool::Select:
                if (event->modifiers() & Qt::ControlModifier) {
                    // Ctrl+Click: Add to selection
                    Voxel* clickedVoxel = getVoxelAt(snapPos);
                    if (clickedVoxel) {
                        if (clickedVoxel->selected) {
                            // Deselect if already selected
                            clickedVoxel->selected = false;
                            if (m_selectedVoxel == clickedVoxel) {
                                m_selectedVoxel = nullptr;
                                emit voxelDeselected();
                            }
                        } else {
                            // Add to selection
                            clickedVoxel->selected = true;
                            m_selectedVoxel = clickedVoxel;
                            emit voxelSelected(clickedVoxel);
                        }
                        update();
                    }
                } else if (event->modifiers() & Qt::ShiftModifier) {
                    // Shift+Click: Range selection
                    if (m_selectedVoxel) {
                        // Select all voxels in a box between current selection and clicked position
                        glm::vec3 min = glm::min(m_selectedVoxel->position, snapPos);
                        glm::vec3 max = glm::max(m_selectedVoxel->position, snapPos);
                        SelectInBox(min, max);
                    } else {
                        SelectVoxel(snapPos);
                    }
                } else {
                    // Normal click: Single selection
                    ClearSelection();
                    SelectVoxel(snapPos);
                }
                break;

            case VoxelTool::Paint:
                {
                    Voxel* clickedVoxel = getVoxelAt(snapPos);
                    if (clickedVoxel) {
                        clickedVoxel->color = m_voxelColor;
                        emit sceneModified();
                        update();
                    }
                }
                break;

            case VoxelTool::Brush:
                {
                    // Use brush to paint multiple voxels
                    BrushPaint(snapPos, m_voxelColor);
                }
                break;

            case VoxelTool::FloodFill:
                {
                    // Flood fill with current color
                    FloodFill(snapPos, m_voxelColor);
                }
                break;

            case VoxelTool::Line:
                {
                    // Start line drawing (store start point)
                    if (!m_isDragging) {
                        m_dragStartPos = snapPos;
                        m_isDragging = true;
                    }
                }
                break;

            case VoxelTool::Rectangle:
                {
                    // Start rectangle drawing
                    if (!m_isDragging) {
                        m_dragStartPos = snapPos;
                        m_isDragging = true;
                    }
                }
                break;

            case VoxelTool::Sphere:
                {
                    // Draw sphere at click position
                    float radius = m_brushSettings.size;
                    DrawSphere(snapPos, radius, m_voxelColor, m_voxelSize, true);
                }
                break;

            case VoxelTool::Copy:
                {
                    // Start copy selection
                    if (!m_isDragging) {
                        m_dragStartPos = snapPos;
                        m_isDragging = true;
                    }
                }
                break;

            case VoxelTool::Paste:
                {
                    // Paste at click position
                    if (HasClipboardData()) {
                        Paste(snapPos, true, false);
                    }
                }
                break;

            default:
                // For other tools, use default behavior
                SelectVoxel(snapPos);
                break;
        }
    } else if (event->button() == Qt::RightButton) {
        // Right click behavior depends on tool and modifiers
        if (m_currentTool == VoxelTool::Select && !(event->modifiers() & Qt::AltModifier)) {
            // Right click in select mode without Alt: context menu
            showContextMenu(event->pos());
        } else {
            // Right click with Alt or other tools: start orbiting
            m_isOrbiting = true;
            setCursor(Qt::ClosedHandCursor);
        }
    } else if (event->button() == Qt::MiddleButton) {
        // Middle mouse: start panning
        m_isPanning = true;
        setCursor(Qt::SizeAllCursor);
    }
}

void VoxelCanvas::mouseMoveEvent(QMouseEvent* event) {
    // Handle gizmo interaction
    if (m_gizmoInteracting) {
        UpdateGizmoInteraction(event->pos());
        m_lastMousePos = event->pos();
        return;
    }

    // Update gizmo hover state
    if (m_currentTool == VoxelTool::Select && !GetSelectedVoxels().empty()) {
        GizmoAxis newHoveredAxis = GetHoveredGizmoAxis(event->pos());
        if (newHoveredAxis != m_hoveredGizmoAxis) {
            m_hoveredGizmoAxis = newHoveredAxis;
            update();
        }
    }

    // Handle camera controls
    QPoint delta = event->pos() - m_lastMousePos;

    if (m_isPanning && (event->buttons() & Qt::MiddleButton)) {
        // Pan camera - move camera position and target together
        float panScale = m_cameraDistance * 0.001f; // Scale by distance for consistent feel

        // Calculate right and up vectors for camera-relative panning
        glm::vec3 forward = glm::normalize(m_cameraTarget - m_cameraPosition);
        glm::vec3 right = glm::normalize(glm::cross(forward, m_cameraUp));
        glm::vec3 up = glm::normalize(glm::cross(right, forward));

        // Apply panning movement
        glm::vec3 panMovement = right * (-delta.x() * panScale) + up * (delta.y() * panScale);
        m_cameraPosition += panMovement;
        m_cameraTarget += panMovement;

        updateCamera();
        update();
    } else if (m_isOrbiting && (event->buttons() & Qt::RightButton)) {
        // Orbit camera around target
        m_cameraYaw += delta.x() * 0.5f;
        m_cameraPitch -= delta.y() * 0.5f;

        // Clamp pitch
        m_cameraPitch = qBound(-89.0f, m_cameraPitch, 89.0f);

        updateCamera();
        update();
    }

    // Update preview
    updatePreview(event->pos());
    
    m_lastMousePos = event->pos();
}

void VoxelCanvas::mouseReleaseEvent(QMouseEvent* event) {
    // Handle gizmo interaction end
    if (m_gizmoInteracting && event->button() == Qt::LeftButton) {
        EndGizmoInteraction();
        m_mousePressed = false;
        return;
    }

    if (m_isDragging && event->button() == Qt::LeftButton) {
        glm::vec3 worldPos = screenToWorld(event->pos());
        glm::vec3 snapPos = getSnapPosition(worldPos);

        // Complete drag operations
        switch (m_currentTool) {
            case VoxelTool::Line:
                {
                    // Draw line from start to end
                    DrawLine(m_dragStartPos, snapPos, m_voxelColor, m_voxelSize);
                }
                break;

            case VoxelTool::Rectangle:
                {
                    // Draw rectangle from start to end
                    DrawRectangle(m_dragStartPos, snapPos, m_voxelColor, m_voxelSize, false);
                }
                break;

            case VoxelTool::Copy:
                {
                    // Copy selection area
                    glm::vec3 minPos = glm::min(m_dragStartPos, snapPos);
                    glm::vec3 maxPos = glm::max(m_dragStartPos, snapPos);
                    CopySelection(minPos, maxPos, true, false);
                }
                break;

            default:
                break;
        }

        m_isDragging = false;
    }

    // Handle camera control release
    if (event->button() == Qt::MiddleButton && m_isPanning) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
    } else if (event->button() == Qt::RightButton && m_isOrbiting) {
        m_isOrbiting = false;
        setCursor(Qt::ArrowCursor);
    }

    m_mousePressed = false;
    m_pressedButton = Qt::NoButton;
}

void VoxelCanvas::wheelEvent(QWheelEvent* event) {
    // Camera zoom
    float delta = event->angleDelta().y() / 120.0f;
    m_cameraDistance -= delta * 0.5f;
    m_cameraDistance = qMax(1.0f, m_cameraDistance);
    
    updateCamera();
    update();
}

void VoxelCanvas::keyPressEvent(QKeyEvent* event) {
    // Handle key repeat for responsive controls
    bool isRepeat = event->isAutoRepeat();

    switch (event->key()) {
        case Qt::Key_1:
            SetPlacementMode(VoxelPlacementMode::GridSnap);
            break;
        case Qt::Key_2:
            SetPlacementMode(VoxelPlacementMode::FaceSnap);
            break;
        case Qt::Key_3:
            SetPlacementMode(VoxelPlacementMode::FreePlace);
            break;
        case Qt::Key_R:
            if (!isRepeat) ResetCamera();
            break;
        case Qt::Key_F:
            if (!isRepeat) FocusOnVoxels();
            break;
        case Qt::Key_Left:
            // Face cycling - works in all modes but most useful in FaceSnap
            m_currentFace = (m_currentFace + 5) % 6; // Previous face (wrap around)
            emit faceChanged(m_currentFace);
            update();
            break;
        case Qt::Key_Right:
            // Face cycling - works in all modes but most useful in FaceSnap
            m_currentFace = (m_currentFace + 1) % 6; // Next face
            emit faceChanged(m_currentFace);
            update();
            break;
        case Qt::Key_Up:
            if (event->modifiers() & Qt::ShiftModifier) {
                // Shift+Up: Raise grid base Y by voxel size at current scale
                float moveAmount = m_voxelSize;
                SetGridBaseY(m_gridBaseY + moveAmount);
                emit gridBaseYChanged(m_gridBaseY);
                update();
            } else {
                // Up arrow: cycle to Y+ face or previous face if already on Y faces
                if (m_currentFace == 2) {
                    // Already on +Y, cycle to next face
                    m_currentFace = (m_currentFace + 1) % 6;
                } else if (m_currentFace == 3) {
                    // On -Y, go to +Y
                    m_currentFace = 2;
                } else {
                    // Not on Y faces, go to +Y
                    m_currentFace = 2;
                }
                emit faceChanged(m_currentFace);
                update();
            }
            break;
        case Qt::Key_Down:
            if (event->modifiers() & Qt::ShiftModifier) {
                // Shift+Down: Lower grid base Y by voxel size at current scale
                float moveAmount = m_voxelSize;
                SetGridBaseY(m_gridBaseY - moveAmount);
                emit gridBaseYChanged(m_gridBaseY);
                update();
            } else {
                // Down arrow: cycle to Y- face or next face if already on Y faces
                if (m_currentFace == 3) {
                    // Already on -Y, cycle to next face
                    m_currentFace = (m_currentFace + 1) % 6;
                } else if (m_currentFace == 2) {
                    // On +Y, go to -Y
                    m_currentFace = 3;
                } else {
                    // Not on Y faces, go to -Y
                    m_currentFace = 3;
                }
                emit faceChanged(m_currentFace);
                update();
            }
            break;
        case Qt::Key_Delete:
        case Qt::Key_Backspace:
            // Delete selected voxels
            if (GetSelectedVoxelCount() > 0) {
                DeleteSelectedVoxels();
            }
            break;
        case Qt::Key_Escape:
            // Clear selection
            if (m_selectedVoxel) {
                m_selectedVoxel = nullptr;
                update();
            }
            break;
        case Qt::Key_G:
            // Toggle grid visibility
            if (!isRepeat) {
                m_showGrid = !m_showGrid;
                update();
            }
            break;
        case Qt::Key_Tab:
            // Cycle through placement modes
            if (!isRepeat) {
                int currentMode = static_cast<int>(m_placementMode);
                currentMode = (currentMode + 1) % 3;
                SetPlacementMode(static_cast<VoxelPlacementMode>(currentMode));
            }
            break;
        case Qt::Key_A:
            if (!isRepeat && (event->modifiers() & Qt::ControlModifier)) {
                // Ctrl+A = Select All
                SelectAll();
            }
            break;
        case Qt::Key_D:
            if (!isRepeat && (event->modifiers() & Qt::ControlModifier)) {
                // Ctrl+D = Deselect All
                ClearSelection();
            }
            break;
        case Qt::Key_I:
            if (!isRepeat && (event->modifiers() & Qt::ControlModifier)) {
                // Ctrl+I = Invert Selection
                InvertSelection();
            }
            break;
        case Qt::Key_Z:
            if (!isRepeat && (event->modifiers() & Qt::ControlModifier)) {
                if (event->modifiers() & Qt::ShiftModifier) {
                    // Ctrl+Shift+Z = Redo
                    Redo();
                } else {
                    // Ctrl+Z = Undo
                    Undo();
                }
            }
            break;
        case Qt::Key_Y:
            if (!isRepeat && (event->modifiers() & Qt::ControlModifier)) {
                // Ctrl+Y = Redo (alternative)
                Redo();
            }
            break;
        case Qt::Key_C:
            if (!isRepeat && (event->modifiers() & Qt::ControlModifier)) {
                // Ctrl+C = Copy selected voxels
                CopySelectedVoxels(true, false);
            }
            break;
        case Qt::Key_V:
            if (!isRepeat && (event->modifiers() & Qt::ControlModifier)) {
                // Ctrl+V = Paste at cursor position
                if (HasClipboardData()) {
                    // Use camera target as paste position
                    Paste(m_cameraTarget, true, false);
                }
            }
            break;
        default:
            QOpenGLWidget::keyPressEvent(event);
            break;
    }
}

void VoxelCanvas::updateCamera() {
    // Convert spherical coordinates to cartesian
    float yawRad = glm::radians(m_cameraYaw);
    float pitchRad = glm::radians(m_cameraPitch);

    glm::vec3 offset;
    offset.x = cos(pitchRad) * cos(yawRad);
    offset.y = sin(pitchRad);
    offset.z = cos(pitchRad) * sin(yawRad);

    m_cameraPosition = m_cameraTarget + offset * m_cameraDistance;

    m_viewMatrix = glm::lookAt(m_cameraPosition, m_cameraTarget, m_cameraUp);
}

void VoxelCanvas::updatePreview(const QPoint& mousePos) {
    glm::vec3 worldPos = screenToWorld(mousePos);
    m_previewPosition = getSnapPosition(worldPos);
    m_showPreview = true;
    update();
}

// Modern OpenGL implementations
void VoxelCanvas::setupShaders() {
    // Vertex shader with vertex colors
    std::string vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aColor;
        layout (location = 2) in vec3 aNormal;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        out vec3 vertexColor;
        out vec3 normal;
        out vec3 fragPos;

        void main() {
            vertexColor = aColor;
            normal = mat3(transpose(inverse(model))) * aNormal;
            fragPos = vec3(model * vec4(aPos, 1.0));
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";

    // Fragment shader with basic lighting
    std::string fragmentShaderSource = R"(
        #version 330 core
        in vec3 vertexColor;
        in vec3 normal;
        in vec3 fragPos;

        out vec4 FragColor;

        uniform vec3 lightPos;
        uniform vec3 lightColor;
        uniform vec3 viewPos;

        void main() {
            // Ambient lighting
            float ambientStrength = 0.3;
            vec3 ambient = ambientStrength * lightColor;

            // Diffuse lighting
            vec3 norm = normalize(normal);
            vec3 lightDir = normalize(lightPos - fragPos);
            float diff = max(dot(norm, lightDir), 0.0);
            vec3 diffuse = diff * lightColor;

            // Specular lighting
            float specularStrength = 0.5;
            vec3 viewDir = normalize(viewPos - fragPos);
            vec3 reflectDir = reflect(-lightDir, norm);
            float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
            vec3 specular = specularStrength * spec * lightColor;

            vec3 result = (ambient + diffuse + specular) * vertexColor;
            FragColor = vec4(result, 1.0);
        }
    )";

    // Compile vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const char* vShaderCode = vertexShaderSource.c_str();
    glShaderSource(vertexShader, 1, &vShaderCode, NULL);
    glCompileShader(vertexShader);

    // Check for vertex shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        qCritical() << "Vertex shader compilation failed:" << infoLog;
    }

    // Compile fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fShaderCode = fragmentShaderSource.c_str();
    glShaderSource(fragmentShader, 1, &fShaderCode, NULL);
    glCompileShader(fragmentShader);

    // Check for fragment shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        qCritical() << "Fragment shader compilation failed:" << infoLog;
    }

    // Create shader program
    m_shaderProgram = glCreateProgram();
    glAttachShader(m_shaderProgram, vertexShader);
    glAttachShader(m_shaderProgram, fragmentShader);
    glLinkProgram(m_shaderProgram);

    // Check for linking errors
    glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(m_shaderProgram, 512, NULL, infoLog);
        qCritical() << "Shader program linking failed:" << infoLog;
    }

    // Clean up shaders
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void VoxelCanvas::setupBuffers() {
    try {
        // Generate VAO, VBO, and EBO for cube rendering
        glGenVertexArrays(1, &m_cubeVAO);
        glGenBuffers(1, &m_cubeVBO);
        glGenBuffers(1, &m_cubeEBO);

        // Check for OpenGL errors after buffer generation
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            qCritical() << "OpenGL error during buffer generation in VoxelCanvas:" << error;
            return;
        }

        // Validate buffer IDs
        if (m_cubeVAO == 0 || m_cubeVBO == 0 || m_cubeEBO == 0) {
            qCritical() << "Failed to generate OpenGL buffers in VoxelCanvas";
            return;
        }

    // Cube vertices with positions, colors (will be set per instance), and normals
    float cubeVertices[] = {
        // Positions          // Normals
        // Front face
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        // Back face
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        // Left face
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        // Right face
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
        // Bottom face
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
        // Top face
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f
    };

    unsigned int cubeIndices[] = {
        // Front face
        0, 1, 2, 2, 3, 0,
        // Back face
        4, 5, 6, 6, 7, 4,
        // Left face
        8, 9, 10, 10, 11, 8,
        // Right face
        12, 13, 14, 14, 15, 12,
        // Bottom face
        16, 17, 18, 18, 19, 16,
        // Top face
        20, 21, 22, 22, 23, 20
    };

    glBindVertexArray(m_cubeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_cubeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal attribute
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    // Setup grid VAO
    glGenVertexArrays(1, &m_gridVAO);
    glGenBuffers(1, &m_gridVBO);

    // Final error check
    error = glGetError();
    if (error != GL_NO_ERROR) {
        qCritical() << "OpenGL error at end of setupBuffers in VoxelCanvas:" << error;
        return;
    }

    qDebug() << "VoxelCanvas buffers setup successfully";
    }
    catch (const std::exception& e) {
        qCritical() << "Exception in VoxelCanvas::setupBuffers:" << e.what();
    }
    catch (...) {
        qCritical() << "Unknown exception in VoxelCanvas::setupBuffers";
    }
}

void VoxelCanvas::drawGrid() {
    // Simple grid rendering using immediate mode for now
    // TODO: Convert to modern OpenGL with VAO/VBO if needed

    // Save current OpenGL state
    GLint currentProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);

    glUseProgram(0); // Use fixed function pipeline for grid

    // Disable lighting and set line properties
    glDisable(GL_LIGHTING);
    glColor3f(0.3f, 0.3f, 0.3f);
    glLineWidth(1.0f);

    // Set matrices for fixed function pipeline
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadMatrixf(glm::value_ptr(m_projectionMatrix));

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadMatrixf(glm::value_ptr(m_viewMatrix));

    glBegin(GL_LINES);

    float gridExtent = 10.0f;
    for (float i = -gridExtent; i <= gridExtent; i += m_gridSize) {
        // X lines (use grid base Y for height)
        glVertex3f(i, m_gridBaseY, -gridExtent);
        glVertex3f(i, m_gridBaseY, gridExtent);

        // Z lines (use grid base Y for height)
        glVertex3f(-gridExtent, m_gridBaseY, i);
        glVertex3f(gridExtent, m_gridBaseY, i);
    }

    glEnd();

    // Restore matrices
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    // Restore OpenGL state
    glEnable(GL_LIGHTING);
    glUseProgram(currentProgram);
}

void VoxelCanvas::drawVoxels() {
    if (m_voxels.empty()) return;

    // Safety check: ensure shader program is valid
    if (m_shaderProgram == 0) {
        return;
    }

    glUseProgram(m_shaderProgram);

    // Set uniforms
    int modelLoc = glGetUniformLocation(m_shaderProgram, "model");
    int viewLoc = glGetUniformLocation(m_shaderProgram, "view");
    int projLoc = glGetUniformLocation(m_shaderProgram, "projection");
    int lightPosLoc = glGetUniformLocation(m_shaderProgram, "lightPos");
    int lightColorLoc = glGetUniformLocation(m_shaderProgram, "lightColor");
    int viewPosLoc = glGetUniformLocation(m_shaderProgram, "viewPos");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(m_viewMatrix));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(m_projectionMatrix));

    // Set lighting uniforms
    glm::vec3 lightPos = m_cameraPosition + glm::vec3(2.0f, 2.0f, 2.0f);
    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
    glUniform3fv(viewPosLoc, 1, glm::value_ptr(m_cameraPosition));

    // Safety check: ensure VAO is valid
    if (m_cubeVAO == 0) {
        return;
    }

    glBindVertexArray(m_cubeVAO);

    for (const auto& voxel : m_voxels) {
        // Create model matrix for this voxel
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, voxel.position);
        model = glm::scale(model, glm::vec3(voxel.size));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        // Set vertex colors for this voxel
        float r = voxel.color.redF();
        float g = voxel.color.greenF();
        float b = voxel.color.blueF();

        // Brighten selected voxels
        if (voxel.selected) {
            r = std::min(1.0f, r + 0.3f);
            g = std::min(1.0f, g + 0.3f);
            b = std::min(1.0f, b + 0.3f);
        }

        // Use instanced rendering approach - bind color data once per voxel
        if (m_colorVBO == 0) {
            glGenBuffers(1, &m_colorVBO);
        }

        // Create vertex color data for all vertices of the cube
        std::vector<float> colorData;
        for (int i = 0; i < 24; ++i) { // 24 vertices (6 faces * 4 vertices)
            colorData.push_back(r);
            colorData.push_back(g);
            colorData.push_back(b);
        }

        glBindBuffer(GL_ARRAY_BUFFER, m_colorVBO);
        glBufferData(GL_ARRAY_BUFFER, colorData.size() * sizeof(float), colorData.data(), GL_DYNAMIC_DRAW);

        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);

        // Draw the cube
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        // Draw selection outline for selected voxels
        if (voxel.selected) {
            // Save current state
            GLint currentPolygonMode[2];
            glGetIntegerv(GL_POLYGON_MODE, currentPolygonMode);
            GLfloat currentLineWidth;
            glGetFloatv(GL_LINE_WIDTH, &currentLineWidth);

            // Set wireframe mode and thicker lines
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glLineWidth(3.0f);

            // Use bright selection color
            std::vector<float> selectionColorData;
            for (int i = 0; i < 24; ++i) {
                selectionColorData.push_back(1.0f); // Bright white outline
                selectionColorData.push_back(1.0f);
                selectionColorData.push_back(0.0f); // Yellow tint
            }

            glBindBuffer(GL_ARRAY_BUFFER, m_colorVBO);
            glBufferData(GL_ARRAY_BUFFER, selectionColorData.size() * sizeof(float), selectionColorData.data(), GL_DYNAMIC_DRAW);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

            // Draw wireframe outline
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

            // Restore state
            glPolygonMode(GL_FRONT_AND_BACK, currentPolygonMode[0]);
            glLineWidth(currentLineWidth);
        }
    }

    glBindVertexArray(0);
}

void VoxelCanvas::drawPreviewVoxel() {
    if (!m_showPreview) return;

    // Safety check: ensure shader program is valid
    if (m_shaderProgram == 0) {
        return;
    }

    glUseProgram(m_shaderProgram);

    // Set uniforms
    int modelLoc = glGetUniformLocation(m_shaderProgram, "model");
    int viewLoc = glGetUniformLocation(m_shaderProgram, "view");
    int projLoc = glGetUniformLocation(m_shaderProgram, "projection");
    int lightPosLoc = glGetUniformLocation(m_shaderProgram, "lightPos");
    int lightColorLoc = glGetUniformLocation(m_shaderProgram, "lightColor");
    int viewPosLoc = glGetUniformLocation(m_shaderProgram, "viewPos");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(m_viewMatrix));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(m_projectionMatrix));

    // Set lighting uniforms
    glm::vec3 lightPos = m_cameraPosition + glm::vec3(2.0f, 2.0f, 2.0f);
    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
    glUniform3fv(viewPosLoc, 1, glm::value_ptr(m_cameraPosition));

    // Create model matrix for preview voxel
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, m_previewPosition);
    model = glm::scale(model, glm::vec3(m_voxelSize));

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    // Set preview color (semi-transparent version of current color)
    float r = m_voxelColor.redF();
    float g = m_voxelColor.greenF();
    float b = m_voxelColor.blueF();

    // Create vertex color data for all vertices of the cube
    std::vector<float> colorData;
    for (int i = 0; i < 24; ++i) { // 24 vertices (6 faces * 4 vertices)
        colorData.push_back(r * 0.7f); // Slightly dimmed for preview
        colorData.push_back(g * 0.7f);
        colorData.push_back(b * 0.7f);
    }

    // Safety check: ensure VAO is valid
    if (m_cubeVAO == 0) {
        return;
    }

    glBindVertexArray(m_cubeVAO);

    // Update color attribute using the shared color VBO
    if (m_colorVBO == 0) {
        glGenBuffers(1, &m_colorVBO);
    }
    glBindBuffer(GL_ARRAY_BUFFER, m_colorVBO);
    glBufferData(GL_ARRAY_BUFFER, colorData.size() * sizeof(float), colorData.data(), GL_DYNAMIC_DRAW);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Draw the preview cube with wireframe
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glDisable(GL_BLEND);
    glBindVertexArray(0);
}

void VoxelCanvas::drawBones() {
    if (m_bones.empty()) return;

    // Disable depth testing for bone rendering
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Use immediate mode for simple bone rendering
    glUseProgram(0);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(m_projectionMatrix));
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(glm::value_ptr(m_viewMatrix));

    glLineWidth(3.0f);
    glPointSize(8.0f);

    // Draw bones
    for (const auto& bone : m_bones) {
        if (!bone.visible) continue;

        // Set bone color
        QColor color = bone.debugColor;
        if (m_selectedBone && m_selectedBone->id == bone.id) {
            color = Qt::white; // Highlight selected bone
        }

        glColor4f(color.redF(), color.greenF(), color.blueF(), 0.8f);

        // Draw bone as a stick (like Blender)
        glm::vec3 boneEnd = bone.position;
        glm::vec3 boneStart = bone.position;

        // Calculate bone direction and length
        float boneLength = 1.0f; // Default bone length
        glm::vec3 boneDirection = glm::vec3(0, 1, 0); // Default upward direction

        // If bone has children, point towards the first child
        bool hasChildren = false;
        for (const auto& otherBone : m_bones) {
            if (otherBone.parentId == bone.id) {
                boneDirection = glm::normalize(otherBone.position - bone.position);
                boneLength = glm::length(otherBone.position - bone.position);
                hasChildren = true;
                break;
            }
        }

        // If no children, use a default length and direction
        if (!hasChildren) {
            boneLength = m_voxelSize * 2.0f; // Scale with voxel size
        }

        boneEnd = boneStart + boneDirection * boneLength;

        // Draw bone shaft as a line
        glLineWidth(4.0f);
        glBegin(GL_LINES);
        glVertex3f(boneStart.x, boneStart.y, boneStart.z);
        glVertex3f(boneEnd.x, boneEnd.y, boneEnd.z);
        glEnd();

        // Draw bone head as a larger point
        glPointSize(12.0f);
        glBegin(GL_POINTS);
        glVertex3f(boneStart.x, boneStart.y, boneStart.z);
        glEnd();

        // Draw bone tail as a smaller point
        glPointSize(8.0f);
        glBegin(GL_POINTS);
        glVertex3f(boneEnd.x, boneEnd.y, boneEnd.z);
        glEnd();

        // Draw connections to parent
        if (bone.parentId != -1) {
            VoxelBone* parent = nullptr;
            for (const auto& b : m_bones) {
                if (b.id == bone.parentId) {
                    parent = const_cast<VoxelBone*>(&b);
                    break;
                }
            }

            if (parent) {
                glBegin(GL_LINES);
                glVertex3f(parent->position.x, parent->position.y, parent->position.z);
                glVertex3f(bone.position.x, bone.position.y, bone.position.z);
                glEnd();
            }
        }

        // Draw bone influence lines to assigned voxels
        glColor4f(color.redF(), color.greenF(), color.blueF(), 0.3f);
        for (const auto& voxel : m_voxels) {
            if (voxel.boneId == bone.id) {
                glBegin(GL_LINES);
                glVertex3f(bone.position.x, bone.position.y, bone.position.z);
                glVertex3f(voxel.position.x, voxel.position.y, voxel.position.z);
                glEnd();
            }
        }
    }

    // Restore OpenGL state
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glLineWidth(1.0f);
    glPointSize(1.0f);
}

void VoxelCanvas::drawGizmos() {
    auto selectedVoxels = GetSelectedVoxels();
    if (selectedVoxels.empty() && !m_selectedBone) {
        return;
    }

    // Calculate gizmo center position
    glm::vec3 gizmoPosition(0.0f);
    if (!selectedVoxels.empty()) {
        for (const auto* voxel : selectedVoxels) {
            gizmoPosition += voxel->position;
        }
        gizmoPosition /= static_cast<float>(selectedVoxels.size());
    } else if (m_selectedBone) {
        gizmoPosition = m_selectedBone->position;
    }

    // Save OpenGL state
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    // Disable depth testing for gizmos so they're always visible
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    // Use immediate mode for gizmos (simpler for now)
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    // Set up view matrix
    glm::mat4 view = glm::lookAt(m_cameraPosition, m_cameraTarget, m_cameraUp);
    glLoadMatrixf(glm::value_ptr(view));

    // Calculate gizmo size based on distance to camera
    float distance = glm::length(m_cameraPosition - gizmoPosition);
    float gizmoScale = distance * 0.1f; // Scale with distance

    glLineWidth(4.0f);

    // Draw gizmo based on current mode
    switch (m_gizmoMode) {
        case GizmoMode::Move:
            drawMoveGizmo(gizmoPosition, gizmoScale);
            break;
        case GizmoMode::Rotate:
            drawRotateGizmo(gizmoPosition, gizmoScale);
            break;
        case GizmoMode::Scale:
            drawScaleGizmo(gizmoPosition, gizmoScale);
            break;
        default:
            drawMoveGizmo(gizmoPosition, gizmoScale);
            break;
    }

    // Restore OpenGL state
    glPopMatrix();
    glPopAttrib();
}

void VoxelCanvas::drawGizmoAxis(const glm::vec3& origin, const glm::vec3& direction, const glm::vec3& color) {
    glm::vec3 end = origin + direction * 2.0f;

    // Simple line rendering using immediate mode for gizmos
    glBegin(GL_LINES);
    glColor3f(color.r, color.g, color.b);
    glVertex3f(origin.x, origin.y, origin.z);
    glVertex3f(end.x, end.y, end.z);
    glEnd();
}

void VoxelCanvas::drawMoveGizmo(const glm::vec3& position, float scale) {
    // X axis (red)
    glColor3f(m_hoveredGizmoAxis == GizmoAxis::X ? 1.0f : 0.8f, 0.0f, 0.0f);
    glBegin(GL_LINES);
    glVertex3f(position.x, position.y, position.z);
    glVertex3f(position.x + scale, position.y, position.z);
    glEnd();

    // X axis arrow head
    glBegin(GL_TRIANGLES);
    glVertex3f(position.x + scale, position.y, position.z);
    glVertex3f(position.x + scale * 0.8f, position.y + scale * 0.1f, position.z);
    glVertex3f(position.x + scale * 0.8f, position.y - scale * 0.1f, position.z);
    glEnd();

    // Y axis (green)
    glColor3f(0.0f, m_hoveredGizmoAxis == GizmoAxis::Y ? 1.0f : 0.8f, 0.0f);
    glBegin(GL_LINES);
    glVertex3f(position.x, position.y, position.z);
    glVertex3f(position.x, position.y + scale, position.z);
    glEnd();

    // Y axis arrow head
    glBegin(GL_TRIANGLES);
    glVertex3f(position.x, position.y + scale, position.z);
    glVertex3f(position.x + scale * 0.1f, position.y + scale * 0.8f, position.z);
    glVertex3f(position.x - scale * 0.1f, position.y + scale * 0.8f, position.z);
    glEnd();

    // Z axis (blue)
    glColor3f(0.0f, 0.0f, m_hoveredGizmoAxis == GizmoAxis::Z ? 1.0f : 0.8f);
    glBegin(GL_LINES);
    glVertex3f(position.x, position.y, position.z);
    glVertex3f(position.x, position.y, position.z + scale);
    glEnd();

    // Z axis arrow head
    glBegin(GL_TRIANGLES);
    glVertex3f(position.x, position.y, position.z + scale);
    glVertex3f(position.x + scale * 0.1f, position.y, position.z + scale * 0.8f);
    glVertex3f(position.x - scale * 0.1f, position.y, position.z + scale * 0.8f);
    glEnd();
}

void VoxelCanvas::drawRotateGizmo(const glm::vec3& position, float scale) {
    // Draw rotation rings for each axis
    const int segments = 32;
    const float radius = scale * 0.8f;

    // X rotation ring (red)
    glColor3f(m_hoveredGizmoAxis == GizmoAxis::X ? 1.0f : 0.8f, 0.0f, 0.0f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        glVertex3f(position.x, position.y + radius * cos(angle), position.z + radius * sin(angle));
    }
    glEnd();

    // Y rotation ring (green)
    glColor3f(0.0f, m_hoveredGizmoAxis == GizmoAxis::Y ? 1.0f : 0.8f, 0.0f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        glVertex3f(position.x + radius * cos(angle), position.y, position.z + radius * sin(angle));
    }
    glEnd();

    // Z rotation ring (blue)
    glColor3f(0.0f, 0.0f, m_hoveredGizmoAxis == GizmoAxis::Z ? 1.0f : 0.8f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        glVertex3f(position.x + radius * cos(angle), position.y + radius * sin(angle), position.z);
    }
    glEnd();
}

void VoxelCanvas::drawScaleGizmo(const glm::vec3& position, float scale) {
    // Draw scale handles as small cubes at the end of each axis
    float handleSize = scale * 0.1f;

    // X axis handle (red)
    glColor3f(m_hoveredGizmoAxis == GizmoAxis::X ? 1.0f : 0.8f, 0.0f, 0.0f);
    glBegin(GL_LINES);
    glVertex3f(position.x, position.y, position.z);
    glVertex3f(position.x + scale, position.y, position.z);
    glEnd();

    // Draw handle cube
    glPushMatrix();
    glTranslatef(position.x + scale, position.y, position.z);
    drawCube(handleSize);
    glPopMatrix();

    // Y axis handle (green)
    glColor3f(0.0f, m_hoveredGizmoAxis == GizmoAxis::Y ? 1.0f : 0.8f, 0.0f);
    glBegin(GL_LINES);
    glVertex3f(position.x, position.y, position.z);
    glVertex3f(position.x, position.y + scale, position.z);
    glEnd();

    glPushMatrix();
    glTranslatef(position.x, position.y + scale, position.z);
    drawCube(handleSize);
    glPopMatrix();

    // Z axis handle (blue)
    glColor3f(0.0f, 0.0f, m_hoveredGizmoAxis == GizmoAxis::Z ? 1.0f : 0.8f);
    glBegin(GL_LINES);
    glVertex3f(position.x, position.y, position.z);
    glVertex3f(position.x, position.y, position.z + scale);
    glEnd();

    glPushMatrix();
    glTranslatef(position.x, position.y, position.z + scale);
    drawCube(handleSize);
    glPopMatrix();
}

void VoxelCanvas::drawCube(float size) {
    float half = size * 0.5f;

    glBegin(GL_QUADS);
    // Front face
    glVertex3f(-half, -half,  half);
    glVertex3f( half, -half,  half);
    glVertex3f( half,  half,  half);
    glVertex3f(-half,  half,  half);

    // Back face
    glVertex3f(-half, -half, -half);
    glVertex3f(-half,  half, -half);
    glVertex3f( half,  half, -half);
    glVertex3f( half, -half, -half);

    // Top face
    glVertex3f(-half,  half, -half);
    glVertex3f(-half,  half,  half);
    glVertex3f( half,  half,  half);
    glVertex3f( half,  half, -half);

    // Bottom face
    glVertex3f(-half, -half, -half);
    glVertex3f( half, -half, -half);
    glVertex3f( half, -half,  half);
    glVertex3f(-half, -half,  half);

    // Right face
    glVertex3f( half, -half, -half);
    glVertex3f( half,  half, -half);
    glVertex3f( half,  half,  half);
    glVertex3f( half, -half,  half);

    // Left face
    glVertex3f(-half, -half, -half);
    glVertex3f(-half, -half,  half);
    glVertex3f(-half,  half,  half);
    glVertex3f(-half,  half, -half);
    glEnd();
}

std::vector<glm::vec3> VoxelCanvas::getSymmetryPositions(const glm::vec3& position) {
    std::vector<glm::vec3> positions;
    positions.push_back(position); // Original position

    if (m_symmetryMode == SymmetryMode::None) {
        return positions;
    }

    glm::vec3 center = m_symmetryCenter;
    glm::vec3 offset = position - center;

    if (m_symmetryMode == SymmetryMode::X) {
        positions.push_back(center + glm::vec3(-offset.x, offset.y, offset.z));
    } else if (m_symmetryMode == SymmetryMode::Y) {
        positions.push_back(center + glm::vec3(offset.x, -offset.y, offset.z));
    } else if (m_symmetryMode == SymmetryMode::Z) {
        positions.push_back(center + glm::vec3(offset.x, offset.y, -offset.z));
    } else if (m_symmetryMode == SymmetryMode::XY) {
        positions.push_back(center + glm::vec3(-offset.x, offset.y, offset.z));
        positions.push_back(center + glm::vec3(offset.x, -offset.y, offset.z));
        positions.push_back(center + glm::vec3(-offset.x, -offset.y, offset.z));
    } else if (m_symmetryMode == SymmetryMode::XZ) {
        positions.push_back(center + glm::vec3(-offset.x, offset.y, offset.z));
        positions.push_back(center + glm::vec3(offset.x, offset.y, -offset.z));
        positions.push_back(center + glm::vec3(-offset.x, offset.y, -offset.z));
    } else if (m_symmetryMode == SymmetryMode::YZ) {
        positions.push_back(center + glm::vec3(offset.x, -offset.y, offset.z));
        positions.push_back(center + glm::vec3(offset.x, offset.y, -offset.z));
        positions.push_back(center + glm::vec3(offset.x, -offset.y, -offset.z));
    } else if (m_symmetryMode == SymmetryMode::XYZ) {
        positions.push_back(center + glm::vec3(-offset.x, offset.y, offset.z));
        positions.push_back(center + glm::vec3(offset.x, -offset.y, offset.z));
        positions.push_back(center + glm::vec3(offset.x, offset.y, -offset.z));
        positions.push_back(center + glm::vec3(-offset.x, -offset.y, offset.z));
        positions.push_back(center + glm::vec3(-offset.x, offset.y, -offset.z));
        positions.push_back(center + glm::vec3(offset.x, -offset.y, -offset.z));
        positions.push_back(center + glm::vec3(-offset.x, -offset.y, -offset.z));
    }

    return positions;
}

glm::vec3 VoxelCanvas::screenToWorld(const QPoint& screenPos) {
    // Convert screen coordinates to normalized device coordinates
    float x = (2.0f * screenPos.x()) / width() - 1.0f;
    float y = 1.0f - (2.0f * screenPos.y()) / height();
    // float z = 1.0f; // Far plane - unused

    // Create the point in NDC space
    glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);

    // Convert to eye coordinates
    glm::mat4 invProjection = glm::inverse(m_projectionMatrix);
    glm::vec4 rayEye = invProjection * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

    // Convert to world coordinates
    glm::mat4 invView = glm::inverse(m_viewMatrix);
    glm::vec4 rayWorld = invView * rayEye;
    glm::vec3 rayDirection = glm::normalize(glm::vec3(rayWorld));

    // Cast ray from camera position in the direction of the mouse
    // For voxel placement, we'll intersect with the grid base Y plane
    glm::vec3 rayOrigin = m_cameraPosition;

    // Calculate intersection with grid base Y plane
    if (abs(rayDirection.y) > 0.001f) {
        float t = (m_gridBaseY - rayOrigin.y) / rayDirection.y;
        if (t > 0) {
            return rayOrigin + t * rayDirection;
        }
    }

    // Fallback: project to a plane at distance from camera
    float distance = 5.0f;
    return rayOrigin + distance * rayDirection;
}

glm::vec3 VoxelCanvas::snapToGrid(const glm::vec3& position) {
    float x = round(position.x / m_gridSize) * m_gridSize;
    float y = round((position.y - m_gridBaseY) / m_gridSize) * m_gridSize + m_gridBaseY;
    float z = round(position.z / m_gridSize) * m_gridSize;
    return glm::vec3(x, y, z);
}

glm::vec3 VoxelCanvas::snapToFace(const glm::vec3& position) {
    // Find nearest voxel and snap to its face
    Voxel* nearestVoxel = nullptr;
    float minDistance = std::numeric_limits<float>::max();

    for (auto& voxel : m_voxels) {
        float distance = glm::distance(position, voxel.position);
        if (distance < minDistance) {
            minDistance = distance;
            nearestVoxel = &voxel;
        }
    }

    if (!nearestVoxel) {
        return snapToGrid(position);
    }

    // Calculate face position based on current face
    glm::vec3 faceOffset;
    switch (m_currentFace) {
        case 0: faceOffset = glm::vec3(1, 0, 0); break;  // +X
        case 1: faceOffset = glm::vec3(-1, 0, 0); break; // -X
        case 2: faceOffset = glm::vec3(0, 1, 0); break;  // +Y
        case 3: faceOffset = glm::vec3(0, -1, 0); break; // -Y
        case 4: faceOffset = glm::vec3(0, 0, 1); break;  // +Z
        case 5: faceOffset = glm::vec3(0, 0, -1); break; // -Z
    }

    return nearestVoxel->position + faceOffset * m_gridSize;
}

Voxel* VoxelCanvas::getVoxelAt(const glm::vec3& position) {
    for (auto& voxel : m_voxels) {
        if (glm::distance(voxel.position, position) < 0.01f) {
            return &voxel;
        }
    }
    return nullptr;
}

glm::vec3 VoxelCanvas::getSnapPosition(const glm::vec3& worldPos) {
    switch (m_placementMode) {
        case VoxelPlacementMode::GridSnap:
            return snapToGrid(worldPos);
        case VoxelPlacementMode::FaceSnap:
            return snapToFace(worldPos);
        case VoxelPlacementMode::FreePlace:
        default:
            return worldPos;
    }
}

// Gizmo operations
GizmoAxis VoxelCanvas::GetHoveredGizmoAxis(const QPoint& screenPos) {
    if (!m_selectedVoxel && GetSelectedVoxels().empty()) {
        return GizmoAxis::None;
    }

    // Get gizmo center position
    glm::vec3 gizmoCenter;
    if (m_selectedVoxel) {
        gizmoCenter = m_selectedVoxel->position;
    } else {
        auto selected = GetSelectedVoxels();
        if (!selected.empty()) {
            gizmoCenter = selected[0]->position;
        }
    }

    // Convert gizmo center to screen space
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::lookAt(m_cameraPosition, m_cameraTarget, m_cameraUp);
    glm::mat4 mvp = m_projectionMatrix * view * model;

    glm::vec4 clipPos = mvp * glm::vec4(gizmoCenter, 1.0f);
    if (clipPos.w <= 0.0f) return GizmoAxis::None;

    glm::vec3 ndcPos = glm::vec3(clipPos) / clipPos.w;
    glm::vec2 screenCenter = glm::vec2(
        (ndcPos.x + 1.0f) * 0.5f * width(),
        (1.0f - ndcPos.y) * 0.5f * height()
    );

    // Check distance to gizmo axes (simplified hit testing)
    float gizmoSize = 50.0f; // pixels
    glm::vec2 mousePos(screenPos.x(), screenPos.y());
    glm::vec2 diff = mousePos - screenCenter;

    if (glm::length(diff) > gizmoSize) {
        return GizmoAxis::None;
    }

    // Simple axis detection based on mouse position relative to center
    if (abs(diff.x) > abs(diff.y)) {
        return diff.x > 0 ? GizmoAxis::X : GizmoAxis::X;
    } else {
        return diff.y > 0 ? GizmoAxis::Y : GizmoAxis::Y;
    }
}

void VoxelCanvas::StartGizmoInteraction(GizmoAxis axis, const QPoint& screenPos) {
    if (axis == GizmoAxis::None) return;

    m_activeGizmoAxis = axis;
    m_gizmoInteracting = true;
    m_gizmoStartPos = screenToWorld(screenPos);
    m_gizmoCurrentPos = m_gizmoStartPos;

    // Store initial positions of selected voxels
    m_selectionStartPositions.clear();
    auto selected = GetSelectedVoxels();
    for (const auto* voxel : selected) {
        m_selectionStartPositions.push_back(voxel->position);
    }

    BeginUndoGroup("Transform Voxels");
}

void VoxelCanvas::UpdateGizmoInteraction(const QPoint& screenPos) {
    if (!m_gizmoInteracting || m_activeGizmoAxis == GizmoAxis::None) return;

    glm::vec3 currentWorldPos = screenToWorld(screenPos);
    glm::vec3 delta = currentWorldPos - m_gizmoStartPos;

    // Constrain movement based on active axis
    switch (m_activeGizmoAxis) {
        case GizmoAxis::X:
            delta = glm::vec3(delta.x, 0.0f, 0.0f);
            break;
        case GizmoAxis::Y:
            delta = glm::vec3(0.0f, delta.y, 0.0f);
            break;
        case GizmoAxis::Z:
            delta = glm::vec3(0.0f, 0.0f, delta.z);
            break;
        default:
            break;
    }

    // Apply transformation to selected voxels
    auto selected = GetSelectedVoxels();
    for (size_t i = 0; i < selected.size() && i < m_selectionStartPositions.size(); ++i) {
        switch (m_gizmoMode) {
            case GizmoMode::Move:
                selected[i]->position = m_selectionStartPositions[i] + delta;
                break;
            case GizmoMode::Scale:
                // Simple uniform scaling for now
                selected[i]->size = std::max(0.1f, selected[i]->size + delta.x * 0.1f);
                break;
            default:
                break;
        }
    }

    m_gizmoCurrentPos = currentWorldPos;
    update();
}

void VoxelCanvas::EndGizmoInteraction() {
    if (m_gizmoInteracting) {
        m_gizmoInteracting = false;
        m_activeGizmoAxis = GizmoAxis::None;
        m_selectionStartPositions.clear();
        EndUndoGroup();
        emit sceneModified();
    }
}

void VoxelCanvas::showContextMenu(const QPoint& pos) {
    auto selectedVoxels = GetSelectedVoxels();
    if (selectedVoxels.empty()) return;

    QMenu contextMenu;

    // Add context menu actions
    QAction* deleteAction = contextMenu.addAction("Delete Selected");
    QAction* duplicateAction = contextMenu.addAction("Duplicate Selected");
    QAction* copyAction = contextMenu.addAction("Copy");
    QAction* cutAction = contextMenu.addAction("Cut");

    contextMenu.addSeparator();

    QAction* selectAllAction = contextMenu.addAction("Select All");
    QAction* deselectAction = contextMenu.addAction("Deselect All");

    // Execute menu and handle actions
    QAction* selectedAction = contextMenu.exec(mapToGlobal(pos));

    if (selectedAction == deleteAction) {
        DeleteSelectedVoxels();
    } else if (selectedAction == duplicateAction) {
        // Duplicate selected voxels
        BeginUndoGroup("Duplicate Voxels");
        glm::vec3 offset(1.0f, 0.0f, 0.0f); // Offset by 1 unit in X
        for (const auto* voxel : selectedVoxels) {
            AddVoxel(voxel->position + offset, voxel->color, voxel->size);
        }
        EndUndoGroup();
        emit sceneModified();
    } else if (selectedAction == copyAction) {
        // Copy to clipboard (simplified)
        m_clipboard.voxels.clear();
        for (const auto* voxel : selectedVoxels) {
            m_clipboard.voxels.push_back(*voxel);
        }
    } else if (selectedAction == cutAction) {
        // Cut to clipboard
        m_clipboard.voxels.clear();
        for (const auto* voxel : selectedVoxels) {
            m_clipboard.voxels.push_back(*voxel);
        }
        DeleteSelectedVoxels();
    } else if (selectedAction == selectAllAction) {
        SelectAll();
    } else if (selectedAction == deselectAction) {
        ClearSelection();
    }
}

// File operations
bool VoxelCanvas::LoadFromFile(const QString& filepath) {
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        return false;
    }

    QJsonObject obj = doc.object();
    QJsonArray voxelsArray = obj["voxels"].toArray();

    ClearVoxels();

    for (const QJsonValue& voxelValue : voxelsArray) {
        QJsonObject voxelObj = voxelValue.toObject();

        glm::vec3 position(
            voxelObj["x"].toDouble(),
            voxelObj["y"].toDouble(),
            voxelObj["z"].toDouble()
        );

        QColor color(
            voxelObj["r"].toInt(),
            voxelObj["g"].toInt(),
            voxelObj["b"].toInt()
        );

        float size = voxelObj["size"].toDouble(1.0);

        AddVoxel(position, color, size);
    }

    return true;
}

bool VoxelCanvas::SaveToFile(const QString& filepath) {
    QJsonObject obj;
    QJsonArray voxelsArray;

    for (const auto& voxel : m_voxels) {
        QJsonObject voxelObj;
        voxelObj["x"] = voxel.position.x;
        voxelObj["y"] = voxel.position.y;
        voxelObj["z"] = voxel.position.z;
        voxelObj["r"] = voxel.color.red();
        voxelObj["g"] = voxel.color.green();
        voxelObj["b"] = voxel.color.blue();
        voxelObj["size"] = voxel.size;

        voxelsArray.append(voxelObj);
    }

    obj["voxels"] = voxelsArray;

    QJsonDocument doc(obj);

    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    file.write(doc.toJson());
    return true;
}

bool VoxelCanvas::ExportToOBJ(const QString& filepath) {
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out << "# Voxel object exported from Lupine Engine\n";
    out << "# Vertex colors are included using extended OBJ format\n";

    int vertexIndex = 1;

    for (const auto& voxel : m_voxels) {
        float x = voxel.position.x;
        float y = voxel.position.y;
        float z = voxel.position.z;
        float s = voxel.size * 0.5f;

        // Get color components (normalized to 0-1 range)
        float r = voxel.color.redF();
        float g = voxel.color.greenF();
        float b = voxel.color.blueF();

        // Write vertices for cube with vertex colors (extended OBJ format)
        out << "v " << (x-s) << " " << (y-s) << " " << (z-s) << " " << r << " " << g << " " << b << "\n";
        out << "v " << (x+s) << " " << (y-s) << " " << (z-s) << " " << r << " " << g << " " << b << "\n";
        out << "v " << (x+s) << " " << (y+s) << " " << (z-s) << " " << r << " " << g << " " << b << "\n";
        out << "v " << (x-s) << " " << (y+s) << " " << (z-s) << " " << r << " " << g << " " << b << "\n";
        out << "v " << (x-s) << " " << (y-s) << " " << (z+s) << " " << r << " " << g << " " << b << "\n";
        out << "v " << (x+s) << " " << (y-s) << " " << (z+s) << " " << r << " " << g << " " << b << "\n";
        out << "v " << (x+s) << " " << (y+s) << " " << (z+s) << " " << r << " " << g << " " << b << "\n";
        out << "v " << (x-s) << " " << (y+s) << " " << (z+s) << " " << r << " " << g << " " << b << "\n";

        // Write faces (counter-clockwise winding for proper normals)
        // Front face (Z+)
        out << "f " << (vertexIndex+4) << " " << (vertexIndex+5) << " " << (vertexIndex+6) << " " << (vertexIndex+7) << "\n";
        // Back face (Z-)
        out << "f " << vertexIndex << " " << (vertexIndex+3) << " " << (vertexIndex+2) << " " << (vertexIndex+1) << "\n";
        // Left face (X-)
        out << "f " << vertexIndex << " " << (vertexIndex+4) << " " << (vertexIndex+7) << " " << (vertexIndex+3) << "\n";
        // Right face (X+)
        out << "f " << (vertexIndex+1) << " " << (vertexIndex+2) << " " << (vertexIndex+6) << " " << (vertexIndex+5) << "\n";
        // Bottom face (Y-)
        out << "f " << vertexIndex << " " << (vertexIndex+1) << " " << (vertexIndex+5) << " " << (vertexIndex+4) << "\n";
        // Top face (Y+)
        out << "f " << (vertexIndex+3) << " " << (vertexIndex+7) << " " << (vertexIndex+6) << " " << (vertexIndex+2) << "\n";

        vertexIndex += 8;
    }

    return true;
}

// Helper function to get UV coordinates for a quad face
std::vector<glm::vec2> VoxelCanvas::getQuadUVs(uint32_t colorRGB, int atlasSize, int textureSize, int totalSize) {
    // Find the color index
    int colorIndex = -1;
    std::vector<uint32_t> uniqueColorRGBs;
    for (const auto& voxel : m_voxels) {
        uint32_t rgb = (voxel.color.red() << 16) | (voxel.color.green() << 8) | voxel.color.blue();
        bool found = false;
        for (const auto& existingRGB : uniqueColorRGBs) {
            if (existingRGB == rgb) {
                found = true;
                break;
            }
        }
        if (!found) {
            uniqueColorRGBs.push_back(rgb);
            if (rgb == colorRGB) {
                colorIndex = static_cast<int>(uniqueColorRGBs.size()) - 1;
            }
        }
    }

    if (colorIndex == -1) {
        // Return default UVs if color not found
        return {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};
    }

    int x = (colorIndex % atlasSize) * textureSize;
    int y = (colorIndex / atlasSize) * textureSize;

    // Add small padding to avoid bleeding
    float padding = 1.0f / totalSize;
    float u_min = (x + padding) / totalSize;
    float v_min = (y + padding) / totalSize;
    float u_max = (x + textureSize - padding) / totalSize;
    float v_max = (y + textureSize - padding) / totalSize;

    // Return UV coordinates for quad corners (counter-clockwise from bottom-left)
    // Note: V coordinates are flipped for proper 3D software compatibility
    return {
        {u_min, 1.0f - v_max}, // Bottom-left
        {u_max, 1.0f - v_max}, // Bottom-right
        {u_max, 1.0f - v_min}, // Top-right
        {u_min, 1.0f - v_min}  // Top-left
    };
}




bool VoxelCanvas::ExportToOBJ(const QString& filepath, bool mergeFaces) {
    return ExportToOBJ(filepath, mergeFaces, false); // Default to vertex colors
}

std::vector<Face> VoxelCanvas::mergeFacesAlgorithm(const std::vector<Face>& faces) {
    std::vector<Face> mergedFaces;
    std::vector<bool> used(faces.size(), false);

    for (size_t i = 0; i < faces.size(); ++i) {
        if (used[i]) continue;

        Face currentFace = faces[i];
        used[i] = true;

        // Try to merge with other faces
        bool merged = true;
        while (merged) {
            merged = false;

            for (size_t j = 0; j < faces.size(); ++j) {
                if (used[j]) continue;

                const Face& otherFace = faces[j];

                // Check if faces can be merged (same normal, color, and coplanar)
                if (canMergeFaces(currentFace, otherFace)) {
                    // Merge the faces
                    Face mergedFace = mergeTwoFaces(currentFace, otherFace);
                    if (mergedFace.vertices[0] != glm::vec3(0.0f)) { // Valid merge
                        currentFace = mergedFace;
                        used[j] = true;
                        merged = true;
                        break;
                    }
                }
            }
        }

        mergedFaces.push_back(currentFace);
    }

    return mergedFaces;
}

bool VoxelCanvas::canMergeFaces(const Face& face1, const Face& face2) {
    // Check if normals are the same
    if (glm::distance(face1.normal, face2.normal) > 0.001f) {
        return false;
    }

    // Check if colors are the same
    if (face1.color != face2.color) {
        return false;
    }

    // Check if faces are coplanar and adjacent
    // For simplicity, check if they share at least one edge
    int sharedVertices = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (glm::distance(face1.vertices[i], face2.vertices[j]) < 0.001f) {
                sharedVertices++;
            }
        }
    }

    // Faces should share exactly 2 vertices (an edge) to be mergeable
    return sharedVertices == 2;
}

Face VoxelCanvas::mergeTwoFaces(const Face& face1, const Face& face2) {
    // Find shared vertices
    std::vector<glm::vec3> sharedVerts;
    std::vector<glm::vec3> uniqueVerts1;
    std::vector<glm::vec3> uniqueVerts2;

    for (int i = 0; i < 4; ++i) {
        bool isShared = false;
        for (int j = 0; j < 4; ++j) {
            if (glm::distance(face1.vertices[i], face2.vertices[j]) < 0.001f) {
                sharedVerts.push_back(face1.vertices[i]);
                isShared = true;
                break;
            }
        }
        if (!isShared) {
            uniqueVerts1.push_back(face1.vertices[i]);
        }
    }

    for (int i = 0; i < 4; ++i) {
        bool isShared = false;
        for (const auto& sharedVert : sharedVerts) {
            if (glm::distance(face2.vertices[i], sharedVert) < 0.001f) {
                isShared = true;
                break;
            }
        }
        if (!isShared) {
            uniqueVerts2.push_back(face2.vertices[i]);
        }
    }

    // If we don't have exactly 2 shared vertices and 2 unique from each face, can't merge
    if (sharedVerts.size() != 2 || uniqueVerts1.size() != 2 || uniqueVerts2.size() != 2) {
        return Face(); // Return invalid face
    }

    // Create merged face with 4 vertices: 2 unique from each face
    Face mergedFace;
    mergedFace.vertices[0] = uniqueVerts1[0];
    mergedFace.vertices[1] = uniqueVerts1[1];
    mergedFace.vertices[2] = uniqueVerts2[0];
    mergedFace.vertices[3] = uniqueVerts2[1];
    mergedFace.normal = face1.normal;
    mergedFace.color = face1.color;
    mergedFace.isExternal = true;

    // Calculate center
    mergedFace.center = (mergedFace.vertices[0] + mergedFace.vertices[1] +
                        mergedFace.vertices[2] + mergedFace.vertices[3]) * 0.25f;

    return mergedFace;
}

bool VoxelCanvas::ExportToOBJ(const QString& filepath, bool mergeFaces, bool useTextureAtlas) {
    if (!mergeFaces && !useTextureAtlas) {
        return ExportToOBJ(filepath); // Use the simple version
    }

    std::map<uint32_t, glm::vec2> colorToUV;
    if (useTextureAtlas) {
        if (!generateTextureAtlas(filepath, colorToUV)) {
            return false;
        }
    }

    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out << "# Voxel object exported from Lupine Engine (Face Merged)\n";

    if (useTextureAtlas) {
        QString textureName = QFileInfo(filepath).baseName() + "_atlas.png";
        QString materialName = QFileInfo(filepath).baseName() + ".mtl";

        out << "# Using texture atlas for colors\n";
        out << "mtllib " << materialName << "\n";

        // Create material file
        QString mtlPath = filepath;
        mtlPath.replace(".obj", ".mtl");
        QFile mtlFile(mtlPath);
        if (mtlFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream mtlOut(&mtlFile);
            mtlOut << "# Material file for voxel export\n";
            mtlOut << "newmtl voxel_material\n";
            mtlOut << "Ka 1.0 1.0 1.0\n";
            mtlOut << "Kd 1.0 1.0 1.0\n";
            mtlOut << "Ks 0.0 0.0 0.0\n";
            mtlOut << "map_Kd " << textureName << "\n";
        }

        out << "usemtl voxel_material\n";
    } else {
        out << "# Vertex colors are included using extended OBJ format\n";
    }

    // Use the global Face struct defined in the header

    std::vector<Face> externalFaces;

    // Generate all faces and determine which are external
    for (const auto& voxel : m_voxels) {
        float x = voxel.position.x;
        float y = voxel.position.y;
        float z = voxel.position.z;
        float s = voxel.size * 0.5f;

        // Define the 6 faces of the cube
        Face faces[6];

        // Front face (Z+)
        faces[0].vertices[0] = glm::vec3(x-s, y-s, z+s);
        faces[0].vertices[1] = glm::vec3(x+s, y-s, z+s);
        faces[0].vertices[2] = glm::vec3(x+s, y+s, z+s);
        faces[0].vertices[3] = glm::vec3(x-s, y+s, z+s);
        faces[0].normal = glm::vec3(0, 0, 1);
        faces[0].center = glm::vec3(x, y, z+s);

        // Back face (Z-)
        faces[1].vertices[0] = glm::vec3(x+s, y-s, z-s);
        faces[1].vertices[1] = glm::vec3(x-s, y-s, z-s);
        faces[1].vertices[2] = glm::vec3(x-s, y+s, z-s);
        faces[1].vertices[3] = glm::vec3(x+s, y+s, z-s);
        faces[1].normal = glm::vec3(0, 0, -1);
        faces[1].center = glm::vec3(x, y, z-s);

        // Right face (X+)
        faces[2].vertices[0] = glm::vec3(x+s, y-s, z+s);
        faces[2].vertices[1] = glm::vec3(x+s, y-s, z-s);
        faces[2].vertices[2] = glm::vec3(x+s, y+s, z-s);
        faces[2].vertices[3] = glm::vec3(x+s, y+s, z+s);
        faces[2].normal = glm::vec3(1, 0, 0);
        faces[2].center = glm::vec3(x+s, y, z);

        // Left face (X-)
        faces[3].vertices[0] = glm::vec3(x-s, y-s, z-s);
        faces[3].vertices[1] = glm::vec3(x-s, y-s, z+s);
        faces[3].vertices[2] = glm::vec3(x-s, y+s, z+s);
        faces[3].vertices[3] = glm::vec3(x-s, y+s, z-s);
        faces[3].normal = glm::vec3(-1, 0, 0);
        faces[3].center = glm::vec3(x-s, y, z);

        // Top face (Y+)
        faces[4].vertices[0] = glm::vec3(x-s, y+s, z+s);
        faces[4].vertices[1] = glm::vec3(x+s, y+s, z+s);
        faces[4].vertices[2] = glm::vec3(x+s, y+s, z-s);
        faces[4].vertices[3] = glm::vec3(x-s, y+s, z-s);
        faces[4].normal = glm::vec3(0, 1, 0);
        faces[4].center = glm::vec3(x, y+s, z);

        // Bottom face (Y-)
        faces[5].vertices[0] = glm::vec3(x-s, y-s, z-s);
        faces[5].vertices[1] = glm::vec3(x+s, y-s, z-s);
        faces[5].vertices[2] = glm::vec3(x+s, y-s, z+s);
        faces[5].vertices[3] = glm::vec3(x-s, y-s, z+s);
        faces[5].normal = glm::vec3(0, -1, 0);
        faces[5].center = glm::vec3(x, y-s, z);

        // Check each face to see if it's external
        for (int i = 0; i < 6; ++i) {
            faces[i].color = voxel.color;
            faces[i].isExternal = true;

            // Check if there's an adjacent voxel that would make this face internal
            glm::vec3 adjacentPos = voxel.position + faces[i].normal * voxel.size;

            for (const auto& otherVoxel : m_voxels) {
                if (glm::distance(otherVoxel.position, adjacentPos) < 0.01f) {
                    faces[i].isExternal = false;
                    break;
                }
            }

            if (faces[i].isExternal) {
                externalFaces.push_back(faces[i]);
            }
        }
    }

    // Implement face merging algorithm for coplanar adjacent faces
    std::vector<Face> mergedFaces;
    if (mergeFaces) {
        mergedFaces = mergeFacesAlgorithm(externalFaces);
    } else {
        mergedFaces = externalFaces;
    }

    int vertexIndex = 1;
    int uvIndex = 1;

    for (const auto& face : mergedFaces) {
        if (useTextureAtlas) {
            // Write vertices without colors
            for (int i = 0; i < 4; ++i) {
                out << "v " << face.vertices[i].x << " " << face.vertices[i].y << " " << face.vertices[i].z << "\n";
            }

            // Write UV coordinates for quad corners
            uint32_t rgb = (face.color.red() << 16) | (face.color.green() << 8) | face.color.blue();

            // Calculate atlas parameters (same as in generateTextureAtlas)
            std::vector<uint32_t> uniqueColorRGBs;
            for (const auto& voxel : m_voxels) {
                uint32_t voxelRgb = (voxel.color.red() << 16) | (voxel.color.green() << 8) | voxel.color.blue();
                bool found = false;
                for (const auto& existingRGB : uniqueColorRGBs) {
                    if (existingRGB == voxelRgb) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    uniqueColorRGBs.push_back(voxelRgb);
                }
            }

            int colorCount = static_cast<int>(uniqueColorRGBs.size());
            int atlasSize = static_cast<int>(std::ceil(std::sqrt(colorCount)));
            int textureSize = 64;
            int totalSize = atlasSize * textureSize;

            std::vector<glm::vec2> quadUVs = getQuadUVs(rgb, atlasSize, textureSize, totalSize);
            for (const auto& uv : quadUVs) {
                out << "vt " << uv.x << " " << uv.y << "\n";
            }

            // Write face with UV coordinates (quad)
            out << "f " << vertexIndex << "/" << uvIndex << " "
                << (vertexIndex+1) << "/" << (uvIndex+1) << " "
                << (vertexIndex+2) << "/" << (uvIndex+2) << " "
                << (vertexIndex+3) << "/" << (uvIndex+3) << "\n";
            uvIndex += 4;
        } else {
            // Write vertices with colors (extended OBJ format)
            float r = face.color.redF();
            float g = face.color.greenF();
            float b = face.color.blueF();

            for (int i = 0; i < 4; ++i) {
                out << "v " << face.vertices[i].x << " " << face.vertices[i].y << " " << face.vertices[i].z
                    << " " << r << " " << g << " " << b << "\n";
            }

            // Write face (quad)
            out << "f " << vertexIndex << " " << (vertexIndex+1) << " " << (vertexIndex+2) << " " << (vertexIndex+3) << "\n";
        }
        vertexIndex += 4;
    }

    return true;
}

bool VoxelCanvas::ExportToFBX(const QString& filepath, bool useTextureAtlas) {
    if (m_voxels.empty()) {
        return false;
    }

    // Generate texture atlas if requested
    std::map<uint32_t, glm::vec2> colorToUV;
    if (useTextureAtlas) {
        if (!generateTextureAtlas(filepath, colorToUV)) {
            qDebug() << "Failed to generate texture atlas for FBX export";
            useTextureAtlas = false; // Fall back to vertex colors
        }
    }

    // Create Assimp scene
    aiScene* scene = new aiScene();
    scene->mRootNode = new aiNode();
    scene->mRootNode->mName = "VoxelRoot";

    // Create material
    scene->mNumMaterials = 1;
    scene->mMaterials = new aiMaterial*[1];
    scene->mMaterials[0] = new aiMaterial();

    aiString materialName("VoxelMaterial");
    scene->mMaterials[0]->AddProperty(&materialName, AI_MATKEY_NAME);

    // Add texture if using texture atlas
    if (useTextureAtlas) {
        QString textureName = QFileInfo(filepath).baseName() + "_atlas.png";
        aiString textureFile(textureName.toStdString());
        scene->mMaterials[0]->AddProperty(&textureFile, AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 0));

        // Set material properties for textured material
        aiColor3D diffuse(1.0f, 1.0f, 1.0f);
        scene->mMaterials[0]->AddProperty(&diffuse, 1, AI_MATKEY_COLOR_DIFFUSE);
    }

    // Create mesh
    scene->mNumMeshes = 1;
    scene->mMeshes = new aiMesh*[1];
    scene->mMeshes[0] = new aiMesh();
    aiMesh* mesh = scene->mMeshes[0];

    mesh->mName = "VoxelMesh";
    mesh->mMaterialIndex = 0;

    // Setup bones if they exist
    bool hasBones = !m_bones.empty();
    if (hasBones) {
        mesh->mNumBones = m_bones.size();
        mesh->mBones = new aiBone*[m_bones.size()];

        for (size_t i = 0; i < m_bones.size(); ++i) {
            const VoxelBone& voxelBone = m_bones[i];

            aiBone* bone = new aiBone();
            bone->mName = voxelBone.name.toStdString();

            // Set bone offset matrix (identity for now, could be calculated based on bone position)
            bone->mOffsetMatrix = aiMatrix4x4(); // Identity matrix

            // Count voxels assigned to this bone
            std::vector<unsigned int> vertexWeights;
            for (size_t voxelIdx = 0; voxelIdx < m_voxels.size(); ++voxelIdx) {
                if (m_voxels[voxelIdx].boneId == voxelBone.id) {
                    // Each voxel has 24 vertices, add all of them
                    for (int v = 0; v < 24; ++v) {
                        vertexWeights.push_back(voxelIdx * 24 + v);
                    }
                }
            }

            bone->mNumWeights = vertexWeights.size();
            if (bone->mNumWeights > 0) {
                bone->mWeights = new aiVertexWeight[bone->mNumWeights];
                for (size_t w = 0; w < vertexWeights.size(); ++w) {
                    bone->mWeights[w].mVertexId = vertexWeights[w];
                    bone->mWeights[w].mWeight = 1.0f; // Full weight for voxel-based rigging
                }
            }

            mesh->mBones[i] = bone;
        }
    }

    // Calculate total vertices and faces
    int totalVertices = m_voxels.size() * 24; // 24 vertices per cube (6 faces * 4 vertices)
    int totalFaces = m_voxels.size() * 12;    // 12 triangles per cube (6 faces * 2 triangles)

    // Allocate vertex arrays
    mesh->mNumVertices = totalVertices;
    mesh->mVertices = new aiVector3D[totalVertices];
    mesh->mNormals = new aiVector3D[totalVertices];

    if (useTextureAtlas) {
        // Use UV coordinates for texture atlas
        mesh->mTextureCoords[0] = new aiVector3D[totalVertices];
        mesh->mNumUVComponents[0] = 2; // 2D UV coordinates
    } else {
        // Use vertex colors
        mesh->mColors[0] = new aiColor4D[totalVertices];
    }

    // Allocate face array
    mesh->mNumFaces = totalFaces;
    mesh->mFaces = new aiFace[totalFaces];

    // Define cube vertices (unit cube centered at origin)
    aiVector3D cubeVertices[24] = {
        // Front face
        {-0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f,  0.5f}, { 0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f},
        // Back face
        {-0.5f, -0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f},
        // Top face
        {-0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f,  0.5f}, { 0.5f,  0.5f,  0.5f}, { 0.5f,  0.5f, -0.5f},
        // Bottom face
        {-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f,  0.5f}, {-0.5f, -0.5f,  0.5f},
        // Right face
        { 0.5f, -0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f}, { 0.5f,  0.5f,  0.5f}, { 0.5f, -0.5f,  0.5f},
        // Left face
        {-0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f, -0.5f}
    };

    // Define cube normals
    aiVector3D cubeNormals[24] = {
        // Front face
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
        // Back face
        {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, -1.0f},
        // Top face
        {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
        // Bottom face
        {0.0f, -1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, -1.0f, 0.0f},
        // Right face
        {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f},
        // Left face
        {-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}
    };

    // Define cube UV coordinates (for texture atlas)
    aiVector3D cubeUVs[24] = {
        // Front face
        {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
        // Back face
        {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f},
        // Top face
        {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f},
        // Bottom face
        {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f},
        // Right face
        {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f},
        // Left face
        {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}
    };

    // Fill vertex data
    int vertexIndex = 0;
    int faceIndex = 0;

    for (const auto& voxel : m_voxels) {
        // Transform and add vertices for this voxel
        for (int i = 0; i < 24; ++i) {
            // Scale and translate vertex
            mesh->mVertices[vertexIndex] = aiVector3D(
                cubeVertices[i].x * voxel.size + voxel.position.x,
                cubeVertices[i].y * voxel.size + voxel.position.y,
                cubeVertices[i].z * voxel.size + voxel.position.z
            );

            // Copy normal
            mesh->mNormals[vertexIndex] = cubeNormals[i];

            if (useTextureAtlas) {
                // Calculate UV coordinates for texture atlas
                uint32_t rgb = (voxel.color.red() << 16) | (voxel.color.green() << 8) | voxel.color.blue();
                auto uvIt = colorToUV.find(rgb);
                if (uvIt != colorToUV.end()) {
                    // Get the base UV coordinates for this color
                    glm::vec2 baseUV = uvIt->second;

                    // Calculate texture atlas dimensions
                    int colorCount = static_cast<int>(colorToUV.size());
                    int atlasSize = static_cast<int>(std::ceil(std::sqrt(colorCount)));
                    int textureSize = 64;
                    int totalSize = atlasSize * textureSize;

                    // Scale the cube UV coordinates to fit within the color's atlas square
                    float uvScale = (textureSize - 2.0f) / totalSize; // Account for padding
                    float u = baseUV.x + cubeUVs[i].x * uvScale;
                    float v = baseUV.y + cubeUVs[i].y * uvScale;

                    mesh->mTextureCoords[0][vertexIndex] = aiVector3D(u, v, 0.0f);
                } else {
                    // Fallback UV coordinates
                    mesh->mTextureCoords[0][vertexIndex] = cubeUVs[i];
                }
            } else {
                // Set vertex color
                mesh->mColors[0][vertexIndex] = aiColor4D(
                    voxel.color.redF(),
                    voxel.color.greenF(),
                    voxel.color.blueF(),
                    1.0f
                );
            }

            vertexIndex++;
        }

        // Add faces for this voxel (12 triangles)
        int baseVertex = (vertexIndex - 24);

        // Face indices for cube triangulation
        int faceIndices[12][3] = {
            // Front face
            {0, 1, 2}, {2, 3, 0},
            // Back face
            {4, 5, 6}, {6, 7, 4},
            // Top face
            {8, 9, 10}, {10, 11, 8},
            // Bottom face
            {12, 13, 14}, {14, 15, 12},
            // Right face
            {16, 17, 18}, {18, 19, 16},
            // Left face
            {20, 21, 22}, {22, 23, 20}
        };

        for (int i = 0; i < 12; ++i) {
            mesh->mFaces[faceIndex].mNumIndices = 3;
            mesh->mFaces[faceIndex].mIndices = new unsigned int[3];
            mesh->mFaces[faceIndex].mIndices[0] = baseVertex + faceIndices[i][0];
            mesh->mFaces[faceIndex].mIndices[1] = baseVertex + faceIndices[i][1];
            mesh->mFaces[faceIndex].mIndices[2] = baseVertex + faceIndices[i][2];
            faceIndex++;
        }
    }

    // Set up bone hierarchy in scene nodes
    if (hasBones) {
        // Create bone nodes
        std::map<int, aiNode*> boneNodes;

        // First pass: create all bone nodes
        for (const auto& voxelBone : m_bones) {
            aiNode* boneNode = new aiNode();
            boneNode->mName = voxelBone.name.toStdString(); // Use original bone name

            // Set bone transformation matrix
            aiMatrix4x4 transform;
            transform.a4 = voxelBone.position.x;
            transform.b4 = voxelBone.position.y;
            transform.c4 = voxelBone.position.z;
            boneNode->mTransformation = transform;

            boneNodes[voxelBone.id] = boneNode;
        }

        // Second pass: set up hierarchy
        for (const auto& voxelBone : m_bones) {
            aiNode* boneNode = boneNodes[voxelBone.id];

            if (voxelBone.parentId >= 0 && boneNodes.find(voxelBone.parentId) != boneNodes.end()) {
                // Add as child to parent
                aiNode* parentNode = boneNodes[voxelBone.parentId];

                // Resize parent's children array
                aiNode** newChildren = new aiNode*[parentNode->mNumChildren + 1];
                for (unsigned int i = 0; i < parentNode->mNumChildren; ++i) {
                    newChildren[i] = parentNode->mChildren[i];
                }
                newChildren[parentNode->mNumChildren] = boneNode;

                delete[] parentNode->mChildren;
                parentNode->mChildren = newChildren;
                parentNode->mNumChildren++;

                boneNode->mParent = parentNode;
            } else {
                // Add as child to root
                aiNode** newChildren = new aiNode*[scene->mRootNode->mNumChildren + 1];
                for (unsigned int i = 0; i < scene->mRootNode->mNumChildren; ++i) {
                    newChildren[i] = scene->mRootNode->mChildren[i];
                }
                newChildren[scene->mRootNode->mNumChildren] = boneNode;

                delete[] scene->mRootNode->mChildren;
                scene->mRootNode->mChildren = newChildren;
                scene->mRootNode->mNumChildren++;

                boneNode->mParent = scene->mRootNode;
            }
        }
    }

    // Set up mesh reference in root node
    scene->mRootNode->mNumMeshes = 1;
    scene->mRootNode->mMeshes = new unsigned int[1];
    scene->mRootNode->mMeshes[0] = 0;

    // Add animations if they exist
    if (!m_animations.empty() && hasBones) {
        scene->mNumAnimations = m_animations.size();
        scene->mAnimations = new aiAnimation*[m_animations.size()];

        for (size_t animIdx = 0; animIdx < m_animations.size(); ++animIdx) {
            const VoxelAnimation& voxelAnim = m_animations[animIdx];

            aiAnimation* animation = new aiAnimation();
            animation->mName = voxelAnim.name.toStdString();
            animation->mDuration = voxelAnim.duration;
            animation->mTicksPerSecond = 30.0; // 30 FPS

            // Create animation channels for each bone track
            animation->mNumChannels = voxelAnim.tracks.size();
            animation->mChannels = new aiNodeAnim*[voxelAnim.tracks.size()];

            for (size_t trackIdx = 0; trackIdx < voxelAnim.tracks.size(); ++trackIdx) {
                const BoneAnimationTrack& track = voxelAnim.tracks[trackIdx];

                // Find bone name
                QString boneName;
                for (const auto& bone : m_bones) {
                    if (bone.id == track.boneId) {
                        boneName = bone.name; // Use original bone name
                        break;
                    }
                }

                if (boneName.isEmpty()) continue;

                aiNodeAnim* nodeAnim = new aiNodeAnim();
                nodeAnim->mNodeName = boneName.toStdString();

                // Set up keyframes
                nodeAnim->mNumPositionKeys = track.keyframes.size();
                nodeAnim->mNumRotationKeys = track.keyframes.size();
                nodeAnim->mNumScalingKeys = track.keyframes.size();

                nodeAnim->mPositionKeys = new aiVectorKey[track.keyframes.size()];
                nodeAnim->mRotationKeys = new aiQuatKey[track.keyframes.size()];
                nodeAnim->mScalingKeys = new aiVectorKey[track.keyframes.size()];

                for (size_t keyIdx = 0; keyIdx < track.keyframes.size(); ++keyIdx) {
                    const BoneKeyframe& keyframe = track.keyframes[keyIdx];
                    double time = keyframe.time * animation->mTicksPerSecond;

                    // Position
                    nodeAnim->mPositionKeys[keyIdx].mTime = time;
                    nodeAnim->mPositionKeys[keyIdx].mValue = aiVector3D(
                        keyframe.position.x, keyframe.position.y, keyframe.position.z);

                    // Rotation (convert Euler to quaternion)
                    aiQuaternion quat;
                    quat = aiQuaternion(keyframe.rotation.y, keyframe.rotation.x, keyframe.rotation.z);
                    nodeAnim->mRotationKeys[keyIdx].mTime = time;
                    nodeAnim->mRotationKeys[keyIdx].mValue = quat;

                    // Scale
                    nodeAnim->mScalingKeys[keyIdx].mTime = time;
                    nodeAnim->mScalingKeys[keyIdx].mValue = aiVector3D(
                        keyframe.scale.x, keyframe.scale.y, keyframe.scale.z);
                }

                animation->mChannels[trackIdx] = nodeAnim;
            }

            scene->mAnimations[animIdx] = animation;
        }
    }

    // Export using Assimp
    Assimp::Exporter exporter;

    // Check if FBX format is supported
    bool fbxSupported = false;
    for (size_t i = 0; i < exporter.GetExportFormatCount(); ++i) {
        const aiExportFormatDesc* desc = exporter.GetExportFormatDescription(i);
        if (std::string(desc->id) == "fbx") {
            fbxSupported = true;
            break;
        }
    }

    if (!fbxSupported) {
        qDebug() << "FBX format not supported by Assimp. Available formats:";
        for (size_t i = 0; i < exporter.GetExportFormatCount(); ++i) {
            const aiExportFormatDesc* desc = exporter.GetExportFormatDescription(i);
            qDebug() << " -" << desc->id << ":" << desc->description;
        }
        delete scene;
        return false;
    }

    // Try to export
    aiReturn result = exporter.Export(scene, "fbx", filepath.toStdString());

    if (result != AI_SUCCESS) {
        qDebug() << "FBX export failed:" << exporter.GetErrorString();
    }

    // Cleanup
    delete scene;

    return result == AI_SUCCESS;
}

bool VoxelCanvas::generateTextureAtlas(const QString& basePath, std::map<uint32_t, glm::vec2>& colorToUV) {
    // Collect unique colors
    std::set<uint32_t> uniqueColors;
    for (const auto& voxel : m_voxels) {
        uint32_t rgb = (voxel.color.red() << 16) | (voxel.color.green() << 8) | voxel.color.blue();
        uniqueColors.insert(rgb);
    }

    if (uniqueColors.empty()) {
        return false;
    }

    // Calculate atlas dimensions
    int colorCount = static_cast<int>(uniqueColors.size());
    int atlasSize = static_cast<int>(std::ceil(std::sqrt(colorCount)));
    int textureSize = 64; // Size of each color square
    int totalSize = atlasSize * textureSize;

    // Create texture atlas image
    QImage atlasImage(totalSize, totalSize, QImage::Format_RGBA8888);
    atlasImage.fill(Qt::transparent);

    QPainter painter(&atlasImage);
    painter.setRenderHint(QPainter::Antialiasing, false);

    // Fill atlas with colors and build UV mapping
    int index = 0;
    for (uint32_t rgb : uniqueColors) {
        int row = index / atlasSize;
        int col = index % atlasSize;

        int x = col * textureSize;
        int y = row * textureSize;

        // Create color from RGB
        QColor color((rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);

        // Fill the square with the color (leave 1 pixel border to prevent bleeding)
        painter.fillRect(x + 1, y + 1, textureSize - 2, textureSize - 2, color);

        // Store UV coordinates (normalized to 0-1 range)
        float u = static_cast<float>(x + 1) / totalSize;
        float v = static_cast<float>(y + 1) / totalSize;
        colorToUV[rgb] = glm::vec2(u, v);

        index++;
    }

    painter.end();

    // Save atlas image
    QString atlasPath = QFileInfo(basePath).path() + "/" + QFileInfo(basePath).baseName() + "_atlas.png";
    bool saved = atlasImage.save(atlasPath, "PNG");

    if (saved) {
        qDebug() << "Texture atlas saved to:" << atlasPath;
        qDebug() << "Atlas size:" << totalSize << "x" << totalSize;
        qDebug() << "Colors:" << colorCount;
    }

    return saved;
}

// VoxelBlockerDialog Implementation
VoxelBlockerDialog::VoxelBlockerDialog(QWidget* parent)
    : QMainWindow(parent)
    , m_menuBar(nullptr)
    , m_toolBar(nullptr)
    , m_centralWidget(nullptr)
    , m_canvas(nullptr)
    , m_toolsDock(nullptr)
    , m_animationDock(nullptr)
    , m_timelineDock(nullptr)
    , m_toolPanel(nullptr)
    , m_toolScrollArea(nullptr)
    , m_animationPanel(nullptr)
    , m_animationScrollArea(nullptr)
    , m_timelinePanel(nullptr)
    , m_timelineTree(nullptr)
    , m_addKeyframeButton(nullptr)
    , m_removeKeyframeButton(nullptr)
    , m_timelineSlider(nullptr)
    , m_timelineLabel(nullptr)
    , m_toolsGroup(nullptr)
    , m_placeToolButton(nullptr)
    , m_eraseToolButton(nullptr)
    , m_selectToolButton(nullptr)
    , m_floodFillToolButton(nullptr)
    , m_lineToolButton(nullptr)
    , m_undoAction(nullptr)
    , m_redoAction(nullptr)
    , m_rectangleToolButton(nullptr)
    , m_sphereToolButton(nullptr)
    , m_toolButtonGroup(nullptr)
    , m_gizmoGroup(nullptr)
    , m_moveGizmoButton(nullptr)
    , m_rotateGizmoButton(nullptr)
    , m_scaleGizmoButton(nullptr)
    , m_gizmoButtonGroup(nullptr)
    , m_voxelSizeSlider(nullptr)
    , m_voxelSizeSpinBox(nullptr)
    , m_voxelColorButton(nullptr)
    , m_placementModeCombo(nullptr)
    , m_gridSizeSlider(nullptr)
    , m_gridSizeSpinBox(nullptr)
    , m_showGridCheck(nullptr)
    , m_gridGroup(nullptr)
    , m_gridBaseYLabel(nullptr)
    , m_gridBaseYSpinBox(nullptr)
    , m_gridUpButton(nullptr)
    , m_gridDownButton(nullptr)
    , m_faceGroup(nullptr)
    , m_currentFaceLabel(nullptr)
    , m_faceDisplayLabel(nullptr)
    , m_animationGroup(nullptr)
    , m_riggingModeCheck(nullptr)
    , m_showBonesCheck(nullptr)
    , m_createBoneButton(nullptr)
    , m_deleteBoneButton(nullptr)
    , m_assignBoneButton(nullptr)
    , m_bonesList(nullptr)
    , m_boneTransformGroup(nullptr)
    , m_bonePositionXSpinBox(nullptr)
    , m_bonePositionYSpinBox(nullptr)
    , m_bonePositionZSpinBox(nullptr)
    , m_boneRotationXSpinBox(nullptr)
    , m_boneRotationYSpinBox(nullptr)
    , m_boneRotationZSpinBox(nullptr)
    , m_boneScaleXSpinBox(nullptr)
    , m_boneScaleYSpinBox(nullptr)
    , m_boneScaleZSpinBox(nullptr)
    , m_setKeyframeButton(nullptr)
    , m_deleteKeyframeButton(nullptr)
    , m_animationControlsGroup(nullptr)
    , m_animationsList(nullptr)
    , m_createAnimationButton(nullptr)
    , m_deleteAnimationButton(nullptr)
    , m_playAnimationButton(nullptr)
    , m_stopAnimationButton(nullptr)
    , m_animationTimeSlider(nullptr)
    , m_animationTimeSpinBox(nullptr)
    , m_animationSpeedSpinBox(nullptr)
    , m_animationDurationSpinBox(nullptr)
    , m_animationStatusLabel(nullptr)
    , m_advancedToolsGroup(nullptr)
    , m_symmetryModeCombo(nullptr)
    , m_symmetryCenterXSpinBox(nullptr)
    , m_symmetryCenterYSpinBox(nullptr)
    , m_symmetryCenterZSpinBox(nullptr)
    , m_resetCameraButton(nullptr)
    , m_focusButton(nullptr)
    , m_voxelCountLabel(nullptr)
    , m_positionLabel(nullptr)
    , m_modified(false)
    , m_voxelCount(0)
{
    setWindowTitle("Voxel Blocker");
    setMinimumSize(1200, 800);
    resize(1400, 900);

    try {
        setupUI();
        updateWindowTitle();
        updateVoxelCount();

        // Initialize face display
        if (m_canvas) {
            onFaceChanged(m_canvas->GetCurrentFace());
        } else {
            qCritical() << "Failed to create VoxelCanvas in VoxelBlockerDialog";
        }
    }
    catch (const std::exception& e) {
        qCritical() << "Exception in VoxelBlockerDialog constructor:" << e.what();
    }
    catch (...) {
        qCritical() << "Unknown exception in VoxelBlockerDialog constructor";
    }
}

VoxelBlockerDialog::~VoxelBlockerDialog() {
}

void VoxelBlockerDialog::setupUI() {
    setWindowTitle("Voxel Builder");
    resize(1400, 900);

    setupMenuBar();
    setupMainPanels();
    setupDockWidgets();

    // Connect canvas signals
    if (m_canvas) {
        connect(m_canvas, &VoxelCanvas::voxelAdded, this, &VoxelBlockerDialog::onVoxelAdded);
        connect(m_canvas, &VoxelCanvas::voxelRemoved, this, &VoxelBlockerDialog::onVoxelRemoved);
        connect(m_canvas, &VoxelCanvas::sceneModified, this, &VoxelBlockerDialog::onSceneModified);
        connect(m_canvas, &VoxelCanvas::faceChanged, this, &VoxelBlockerDialog::onFaceChanged);
        connect(m_canvas, &VoxelCanvas::gridBaseYChanged, this, [this](float baseY) {
            m_gridBaseYSpinBox->blockSignals(true);
            m_gridBaseYSpinBox->setValue(baseY);
            m_gridBaseYSpinBox->blockSignals(false);
        });

        // Update undo/redo actions initially
        updateUndoRedoActions();
    }
}

void VoxelBlockerDialog::setupMenuBar() {
    m_menuBar = new QMenuBar(this);
    setMenuBar(m_menuBar);  // Set the menu bar for the QMainWindow

    // File menu
    QMenu* fileMenu = m_menuBar->addMenu("&File");
    fileMenu->addAction("&New Scene", this, &VoxelBlockerDialog::onNewScene, QKeySequence::New);
    fileMenu->addAction("&Open...", this, &VoxelBlockerDialog::onOpenFile, QKeySequence::Open);
    fileMenu->addSeparator();
    fileMenu->addAction("&Save", this, &VoxelBlockerDialog::onSaveFile, QKeySequence::Save);
    fileMenu->addAction("Save &As...", this, &VoxelBlockerDialog::onSaveAs, QKeySequence::SaveAs);
    fileMenu->addSeparator();
    fileMenu->addAction("Export to &OBJ...", this, &VoxelBlockerDialog::onExportOBJ);
    fileMenu->addAction("Export to &FBX...", this, &VoxelBlockerDialog::onExportFBX);
    fileMenu->addSeparator();
    fileMenu->addAction("&Close", this, &QDialog::close, QKeySequence::Close);

    // Edit menu
    QMenu* editMenu = m_menuBar->addMenu("&Edit");
    QAction* undoAction = editMenu->addAction("&Undo", this, &VoxelBlockerDialog::onUndo, QKeySequence::Undo);
    QAction* redoAction = editMenu->addAction("&Redo", this, &VoxelBlockerDialog::onRedo, QKeySequence::Redo);

    // Store actions for enabling/disabling
    m_undoAction = undoAction;
    m_redoAction = redoAction;

    editMenu->addSeparator();
    editMenu->addAction("&Copy", this, &VoxelBlockerDialog::onCopy, QKeySequence::Copy);
    editMenu->addAction("&Paste", this, &VoxelBlockerDialog::onPaste, QKeySequence::Paste);
    editMenu->addAction("Cu&t", this, &VoxelBlockerDialog::onCut, QKeySequence::Cut);

    // View menu
    QMenu* viewMenu = m_menuBar->addMenu("&View");
    QAction* gridAction = viewMenu->addAction("Show &Grid");
    gridAction->setCheckable(true);
    gridAction->setChecked(true);
    connect(gridAction, &QAction::toggled, [this](bool checked) {
        if (m_canvas) m_canvas->SetShowGrid(checked);
        if (m_showGridCheck) m_showGridCheck->setChecked(checked);
    });

    viewMenu->addSeparator();
    viewMenu->addAction("&Reset Camera", this, &VoxelBlockerDialog::onResetCamera, QKeySequence("R"));
    viewMenu->addAction("&Focus on Voxels", this, &VoxelBlockerDialog::onFocusOnVoxels, QKeySequence("F"));
}

void VoxelBlockerDialog::setupToolBar() {
    // Remove the toolbar that was overlapping the file menu
    // The file menu already provides all necessary file operations
    // Camera controls are available through the View menu and keyboard shortcuts
}

void VoxelBlockerDialog::setupMainPanels() {
    // Set up central widget (viewport)
    setupViewportPanel();
}

void VoxelBlockerDialog::setupDockWidgets() {
    // Create dock widgets
    m_toolsDock = new QDockWidget("Tools & Settings", this);
    m_toolsDock->setObjectName("ToolsDock");
    m_toolsDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    m_animationDock = new QDockWidget("Animation", this);
    m_animationDock->setObjectName("AnimationDock");
    m_animationDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    m_timelineDock = new QDockWidget("Timeline", this);
    m_timelineDock->setObjectName("TimelineDock");
    m_timelineDock->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);

    // Setup panels
    setupToolPanel();
    setupAnimationPanel();
    setupTimelinePanel();

    // Add dock widgets to main window
    addDockWidget(Qt::LeftDockWidgetArea, m_toolsDock);
    addDockWidget(Qt::RightDockWidgetArea, m_animationDock);
    addDockWidget(Qt::BottomDockWidgetArea, m_timelineDock);

    // Set initial sizes
    resizeDocks({m_toolsDock}, {350}, Qt::Horizontal);
    resizeDocks({m_animationDock}, {350}, Qt::Horizontal);
    resizeDocks({m_timelineDock}, {200}, Qt::Vertical);

    // Connect dock visibility signals
    connect(m_toolsDock, &QDockWidget::visibilityChanged, this, &VoxelBlockerDialog::onDockVisibilityChanged);
    connect(m_animationDock, &QDockWidget::visibilityChanged, this, &VoxelBlockerDialog::onDockVisibilityChanged);
    connect(m_timelineDock, &QDockWidget::visibilityChanged, this, &VoxelBlockerDialog::onDockVisibilityChanged);
}

void VoxelBlockerDialog::setupToolPanel() {
    // Create scroll area for tools
    m_toolScrollArea = new QScrollArea();
    m_toolScrollArea->setWidgetResizable(true);
    m_toolScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_toolScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // Create tool panel widget
    m_toolPanel = new QWidget();
    QVBoxLayout* toolLayout = new QVBoxLayout(m_toolPanel);
    toolLayout->setSpacing(8);
    toolLayout->setContentsMargins(8, 8, 8, 8);

    // Tools group
    m_toolsGroup = new QGroupBox("Tools");
    m_toolsGroup->setMinimumHeight(150);
    QGridLayout* toolsLayout = new QGridLayout(m_toolsGroup);
    toolsLayout->setSpacing(4);

    // Create tool buttons
    m_placeToolButton = new QPushButton("Place");
    m_eraseToolButton = new QPushButton("Erase");
    m_selectToolButton = new QPushButton("Select");
    m_floodFillToolButton = new QPushButton("Fill");
    m_lineToolButton = new QPushButton("Line");
    m_rectangleToolButton = new QPushButton("Rect");
    m_sphereToolButton = new QPushButton("Sphere");

    // Make buttons checkable and set up button group
    m_toolButtonGroup = new QButtonGroup(this);
    QList<QPushButton*> toolButtons = {
        m_placeToolButton, m_eraseToolButton, m_selectToolButton,
        m_floodFillToolButton, m_lineToolButton, m_rectangleToolButton,
        m_sphereToolButton
    };

    for (int i = 0; i < toolButtons.size(); ++i) {
        QPushButton* button = toolButtons[i];
        button->setCheckable(true);
        button->setMinimumHeight(35);
        button->setMaximumHeight(35);
        button->setMinimumWidth(80);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_toolButtonGroup->addButton(button, i);

        // Arrange in 3 columns
        int row = i / 3;
        int col = i % 3;
        toolsLayout->addWidget(button, row, col);
    }

    // Set Place tool as default
    m_placeToolButton->setChecked(true);

    toolLayout->addWidget(m_toolsGroup);

    // Gizmo mode group (only visible in select mode)
    m_gizmoGroup = new QGroupBox("Transform Mode");
    m_gizmoGroup->setMinimumHeight(80);
    m_gizmoGroup->setVisible(false); // Hidden by default
    QHBoxLayout* gizmoLayout = new QHBoxLayout(m_gizmoGroup);
    gizmoLayout->setSpacing(4);

    // Create gizmo mode buttons
    m_moveGizmoButton = new QPushButton("Move");
    m_rotateGizmoButton = new QPushButton("Rotate");
    m_scaleGizmoButton = new QPushButton("Scale");

    // Make gizmo buttons checkable
    m_moveGizmoButton->setCheckable(true);
    m_rotateGizmoButton->setCheckable(true);
    m_scaleGizmoButton->setCheckable(true);
    m_moveGizmoButton->setChecked(true); // Default to move mode

    // Create button group for gizmo modes
    m_gizmoButtonGroup = new QButtonGroup(this);
    m_gizmoButtonGroup->addButton(m_moveGizmoButton, static_cast<int>(GizmoMode::Move));
    m_gizmoButtonGroup->addButton(m_rotateGizmoButton, static_cast<int>(GizmoMode::Rotate));
    m_gizmoButtonGroup->addButton(m_scaleGizmoButton, static_cast<int>(GizmoMode::Scale));

    gizmoLayout->addWidget(m_moveGizmoButton);
    gizmoLayout->addWidget(m_rotateGizmoButton);
    gizmoLayout->addWidget(m_scaleGizmoButton);

    toolLayout->addWidget(m_gizmoGroup);

    // Voxel settings group
    QGroupBox* voxelGroup = new QGroupBox("Voxel Settings");
    voxelGroup->setMinimumHeight(200);
    QVBoxLayout* voxelLayout = new QVBoxLayout(voxelGroup);
    voxelLayout->setSpacing(6);

    // Voxel size
    QLabel* sizeLabel = new QLabel("Size:");
    voxelLayout->addWidget(sizeLabel);

    QHBoxLayout* sizeLayout = new QHBoxLayout();
    m_voxelSizeSlider = new QSlider(Qt::Horizontal);
    m_voxelSizeSlider->setRange(1, 50);
    m_voxelSizeSlider->setValue(10);
    sizeLayout->addWidget(m_voxelSizeSlider);

    m_voxelSizeSpinBox = new QDoubleSpinBox();
    m_voxelSizeSpinBox->setRange(0.1, 5.0);
    m_voxelSizeSpinBox->setValue(1.0);
    m_voxelSizeSpinBox->setDecimals(1);
    m_voxelSizeSpinBox->setMaximumWidth(80);
    sizeLayout->addWidget(m_voxelSizeSpinBox);

    voxelLayout->addLayout(sizeLayout);

    // Voxel color
    QLabel* colorLabel = new QLabel("Color:");
    voxelLayout->addWidget(colorLabel);

    // Color selection area with preset colors and custom color button
    QFrame* colorFrame = new QFrame();
    colorFrame->setFrameStyle(QFrame::StyledPanel);
    colorFrame->setMinimumHeight(80);
    QVBoxLayout* colorFrameLayout = new QVBoxLayout(colorFrame);
    colorFrameLayout->setSpacing(4);

    // Preset color palette
    QGridLayout* colorPaletteLayout = new QGridLayout();
    colorPaletteLayout->setSpacing(2);

    QList<QColor> presetColors = {
        Qt::red, Qt::green, Qt::blue, Qt::yellow, Qt::cyan, Qt::magenta,
        Qt::white, Qt::lightGray, Qt::gray, Qt::darkGray, Qt::black,
        QColor(255, 165, 0), QColor(128, 0, 128), QColor(255, 192, 203),
        QColor(165, 42, 42), QColor(0, 128, 0), QColor(75, 0, 130)
    };

    for (int i = 0; i < presetColors.size(); ++i) {
        QPushButton* colorBtn = new QPushButton();
        colorBtn->setFixedSize(25, 25);
        colorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid black;").arg(presetColors[i].name()));
        colorBtn->setProperty("color", presetColors[i]);

        connect(colorBtn, &QPushButton::clicked, this, [this, colorBtn]() {
            QColor color = colorBtn->property("color").value<QColor>();
            setVoxelColor(color);
        });

        int row = i / 6;
        int col = i % 6;
        colorPaletteLayout->addWidget(colorBtn, row, col);
    }

    colorFrameLayout->addLayout(colorPaletteLayout);

    // Custom color button and current color display
    QHBoxLayout* customColorLayout = new QHBoxLayout();

    m_voxelColorButton = new QPushButton();
    m_voxelColorButton->setFixedSize(40, 25);
    m_voxelColorButton->setStyleSheet("background-color: red; border: 2px solid gray;");
    customColorLayout->addWidget(m_voxelColorButton);

    QPushButton* customColorBtn = new QPushButton("Custom...");
    customColorBtn->setMinimumHeight(25);
    customColorLayout->addWidget(customColorBtn);

    connect(customColorBtn, &QPushButton::clicked, this, &VoxelBlockerDialog::onVoxelColorChanged);

    colorFrameLayout->addLayout(customColorLayout);
    voxelLayout->addWidget(colorFrame);

    toolLayout->addWidget(voxelGroup);

    // Placement settings group
    QGroupBox* placementGroup = new QGroupBox("Placement");
    placementGroup->setMinimumHeight(120);
    QVBoxLayout* placementLayout = new QVBoxLayout(placementGroup);
    placementLayout->setSpacing(6);

    // Placement mode
    QLabel* modeLabel = new QLabel("Mode:");
    modeLabel->setContentsMargins(0, 8, 0, 4);
    placementLayout->addWidget(modeLabel);

    m_placementModeCombo = new QComboBox();
    m_placementModeCombo->addItems({"Grid Snap", "Face Snap", "Free Place"});
    m_placementModeCombo->setMinimumHeight(30);
    m_placementModeCombo->setMinimumWidth(120);
    m_placementModeCombo->setContentsMargins(0, 4, 0, 8);
    placementLayout->addWidget(m_placementModeCombo);

    // Grid size
    QLabel* gridSizeLabel = new QLabel("Grid Size:");
    gridSizeLabel->setContentsMargins(0, 8, 0, 4);
    placementLayout->addWidget(gridSizeLabel);

    QHBoxLayout* gridSizeLayout = new QHBoxLayout();
    gridSizeLayout->setContentsMargins(0, 4, 0, 8);
    m_gridSizeSlider = new QSlider(Qt::Horizontal);
    m_gridSizeSlider->setRange(1, 50);
    m_gridSizeSlider->setValue(10);
    m_gridSizeSlider->setMinimumHeight(25);
    gridSizeLayout->addWidget(m_gridSizeSlider);

    m_gridSizeSpinBox = new QDoubleSpinBox();
    m_gridSizeSpinBox->setRange(0.1, 5.0);
    m_gridSizeSpinBox->setValue(1.0);
    m_gridSizeSpinBox->setDecimals(1);
    m_gridSizeSpinBox->setMinimumHeight(30);
    m_gridSizeSpinBox->setMinimumWidth(80);
    m_gridSizeSpinBox->setMaximumWidth(80);
    gridSizeLayout->addWidget(m_gridSizeSpinBox);

    placementLayout->addLayout(gridSizeLayout);

    // Show grid
    m_showGridCheck = new QCheckBox("Show Grid");
    m_showGridCheck->setChecked(true);
    placementLayout->addWidget(m_showGridCheck);

    toolLayout->addWidget(placementGroup);

    // Grid controls group
    m_gridGroup = new QGroupBox("Grid Controls");
    m_gridGroup->setMinimumHeight(80);
    QVBoxLayout* gridLayout = new QVBoxLayout(m_gridGroup);
    gridLayout->setSpacing(6);

    // Grid base Y
    QLabel* gridBaseYLabel = new QLabel("Grid Base Y:");
    gridLayout->addWidget(gridBaseYLabel);

    QHBoxLayout* gridBaseYLayout = new QHBoxLayout();
    m_gridBaseYSpinBox = new QDoubleSpinBox();
    m_gridBaseYSpinBox->setRange(-100.0, 100.0);
    m_gridBaseYSpinBox->setValue(0.0);
    m_gridBaseYSpinBox->setDecimals(1);
    m_gridBaseYSpinBox->setSingleStep(0.5);
    gridBaseYLayout->addWidget(m_gridBaseYSpinBox);

    m_gridUpButton = new QPushButton("↑");
    m_gridUpButton->setMaximumWidth(30);
    m_gridUpButton->setToolTip("Shift+Up: Raise grid");
    gridBaseYLayout->addWidget(m_gridUpButton);

    m_gridDownButton = new QPushButton("↓");
    m_gridDownButton->setMaximumWidth(30);
    m_gridDownButton->setToolTip("Shift+Down: Lower grid");
    gridBaseYLayout->addWidget(m_gridDownButton);

    gridLayout->addLayout(gridBaseYLayout);

    toolLayout->addWidget(m_gridGroup);

    // Face cycling group
    m_faceGroup = new QGroupBox("Face Cycling");
    m_faceGroup->setMinimumHeight(80);
    QVBoxLayout* faceLayout = new QVBoxLayout(m_faceGroup);
    faceLayout->setSpacing(4);

    m_currentFaceLabel = new QLabel("Current Face:");
    faceLayout->addWidget(m_currentFaceLabel);

    m_faceDisplayLabel = new QLabel("+X (Right)");
    m_faceDisplayLabel->setStyleSheet("font-weight: bold; color: #4CAF50;");
    faceLayout->addWidget(m_faceDisplayLabel);

    QLabel* faceHelpLabel = new QLabel("Use arrow keys to cycle faces");
    faceHelpLabel->setStyleSheet("font-size: 10px; color: gray;");
    faceLayout->addWidget(faceHelpLabel);

    toolLayout->addWidget(m_faceGroup);

    // Camera controls group (in basic panel)
    QGroupBox* cameraGroup = new QGroupBox("Camera");
    cameraGroup->setMinimumHeight(80);
    QVBoxLayout* cameraLayout = new QVBoxLayout(cameraGroup);
    cameraLayout->setSpacing(6);

    m_resetCameraButton = new QPushButton("Reset Camera");
    m_resetCameraButton->setMinimumHeight(30);
    cameraLayout->addWidget(m_resetCameraButton);

    m_focusButton = new QPushButton("Focus on Voxels");
    m_focusButton->setMinimumHeight(30);
    cameraLayout->addWidget(m_focusButton);

    toolLayout->addWidget(cameraGroup);

    // Info group
    QGroupBox* infoGroup = new QGroupBox("Info");
    infoGroup->setMinimumHeight(60);
    QVBoxLayout* infoLayout = new QVBoxLayout(infoGroup);
    infoLayout->setSpacing(4);

    m_voxelCountLabel = new QLabel("Voxels: 0");
    infoLayout->addWidget(m_voxelCountLabel);

    toolLayout->addWidget(infoGroup);

    // Add stretch to tool panel
    toolLayout->addStretch();

    // Set up scroll area and dock
    m_toolScrollArea->setWidget(m_toolPanel);
    m_toolsDock->setWidget(m_toolScrollArea);

    // Connect tool panel signals
    // Tool selection - map button IDs to correct tool enum values
    connect(m_toolButtonGroup, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked), this, [this](QAbstractButton* button) {
        int buttonId = m_toolButtonGroup->id(button);
        VoxelTool tool;

        // Map button index to correct tool enum
        switch (buttonId) {
            case 0: tool = VoxelTool::Place; break;
            case 1: tool = VoxelTool::Erase; break;
            case 2: tool = VoxelTool::Select; break;
            case 3: tool = VoxelTool::FloodFill; break;
            case 4: tool = VoxelTool::Line; break;
            case 5: tool = VoxelTool::Rectangle; break;
            case 6: tool = VoxelTool::Sphere; break;
            default: tool = VoxelTool::Place; break;
        }

        onToolChanged(static_cast<int>(tool));
    });

    // Gizmo mode selection
    connect(m_gizmoButtonGroup, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked), this, [this](QAbstractButton* button) {
        int gizmoModeId = m_gizmoButtonGroup->id(button);
        onGizmoModeChanged(static_cast<GizmoMode>(gizmoModeId));
    });

    // Voxel settings
    connect(m_voxelSizeSlider, &QSlider::valueChanged, this, &VoxelBlockerDialog::onVoxelSizeChanged);
    connect(m_voxelSizeSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VoxelBlockerDialog::onVoxelSizeChanged);
    connect(m_voxelColorButton, &QPushButton::clicked, this, &VoxelBlockerDialog::onVoxelColorChanged);
    connect(m_placementModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &VoxelBlockerDialog::onPlacementModeChanged);
    connect(m_gridSizeSlider, &QSlider::valueChanged, this, &VoxelBlockerDialog::onGridSizeChanged);
    connect(m_gridSizeSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VoxelBlockerDialog::onGridSizeChanged);
    connect(m_showGridCheck, &QCheckBox::toggled, this, &VoxelBlockerDialog::onShowGridChanged);

    // Grid controls
    connect(m_gridBaseYSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VoxelBlockerDialog::onGridBaseYChanged);
    connect(m_gridUpButton, &QPushButton::clicked, this, &VoxelBlockerDialog::onGridUpClicked);
    connect(m_gridDownButton, &QPushButton::clicked, this, &VoxelBlockerDialog::onGridDownClicked);

    // Camera controls
    connect(m_resetCameraButton, &QPushButton::clicked, this, &VoxelBlockerDialog::onResetCamera);
    connect(m_focusButton, &QPushButton::clicked, this, &VoxelBlockerDialog::onFocusOnVoxels);
}

void VoxelBlockerDialog::setupAnimationPanel() {
    // Create animation panel widget
    m_animationPanel = new QWidget();
    QVBoxLayout* animLayout = new QVBoxLayout(m_animationPanel);
    animLayout->setSpacing(8);
    animLayout->setContentsMargins(8, 8, 8, 8);

    // Animation system group
    m_animationGroup = new QGroupBox("Animation System");
    m_animationGroup->setMinimumHeight(180);
    QVBoxLayout* animationLayout = new QVBoxLayout(m_animationGroup);
    animationLayout->setSpacing(6);

    // Rigging controls
    m_riggingModeCheck = new QCheckBox("Rigging Mode");
    animationLayout->addWidget(m_riggingModeCheck);

    m_showBonesCheck = new QCheckBox("Show Bones");
    m_showBonesCheck->setChecked(true);
    animationLayout->addWidget(m_showBonesCheck);

    // Bone management
    QHBoxLayout* boneButtonLayout = new QHBoxLayout();
    m_createBoneButton = new QPushButton("Create");
    m_createBoneButton->setMinimumHeight(30);
    m_deleteBoneButton = new QPushButton("Delete");
    m_deleteBoneButton->setMinimumHeight(30);
    boneButtonLayout->addWidget(m_createBoneButton);
    boneButtonLayout->addWidget(m_deleteBoneButton);
    animationLayout->addLayout(boneButtonLayout);

    // Bone assignment
    m_assignBoneButton = new QPushButton("Assign Selected Voxels to Bone");
    m_assignBoneButton->setMinimumHeight(30);
    m_assignBoneButton->setEnabled(false); // Enabled when bone and voxels are selected
    animationLayout->addWidget(m_assignBoneButton);

    // Bones list
    QLabel* bonesLabel = new QLabel("Bones:");
    animationLayout->addWidget(bonesLabel);
    m_bonesList = new QListWidget();
    m_bonesList->setMinimumHeight(80);
    m_bonesList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_bonesList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_bonesList, &QListWidget::customContextMenuRequested, this, &VoxelBlockerDialog::showBoneContextMenu);
    animationLayout->addWidget(m_bonesList);

    // Bone transform controls
    m_boneTransformGroup = new QGroupBox("Bone Transform");
    m_boneTransformGroup->setMinimumHeight(200);
    m_boneTransformGroup->setEnabled(false); // Disabled until bone is selected
    QVBoxLayout* transformLayout = new QVBoxLayout(m_boneTransformGroup);
    transformLayout->setSpacing(4);

    // Position controls
    QLabel* posLabel = new QLabel("Position:");
    transformLayout->addWidget(posLabel);
    QHBoxLayout* posLayout = new QHBoxLayout();

    QLabel* posXLabel = new QLabel("X:");
    m_bonePositionXSpinBox = new QDoubleSpinBox();
    m_bonePositionXSpinBox->setRange(-1000.0, 1000.0);
    m_bonePositionXSpinBox->setDecimals(3);
    m_bonePositionXSpinBox->setSingleStep(0.1);
    posLayout->addWidget(posXLabel);
    posLayout->addWidget(m_bonePositionXSpinBox);

    QLabel* posYLabel = new QLabel("Y:");
    m_bonePositionYSpinBox = new QDoubleSpinBox();
    m_bonePositionYSpinBox->setRange(-1000.0, 1000.0);
    m_bonePositionYSpinBox->setDecimals(3);
    m_bonePositionYSpinBox->setSingleStep(0.1);
    posLayout->addWidget(posYLabel);
    posLayout->addWidget(m_bonePositionYSpinBox);

    QLabel* posZLabel = new QLabel("Z:");
    m_bonePositionZSpinBox = new QDoubleSpinBox();
    m_bonePositionZSpinBox->setRange(-1000.0, 1000.0);
    m_bonePositionZSpinBox->setDecimals(3);
    m_bonePositionZSpinBox->setSingleStep(0.1);
    posLayout->addWidget(posZLabel);
    posLayout->addWidget(m_bonePositionZSpinBox);

    transformLayout->addLayout(posLayout);

    // Rotation controls (quaternion as Euler angles for UI)
    QLabel* rotLabel = new QLabel("Rotation (Degrees):");
    transformLayout->addWidget(rotLabel);
    QHBoxLayout* rotLayout = new QHBoxLayout();

    QLabel* rotXLabel = new QLabel("X:");
    m_boneRotationXSpinBox = new QDoubleSpinBox();
    m_boneRotationXSpinBox->setRange(-360.0, 360.0);
    m_boneRotationXSpinBox->setDecimals(2);
    m_boneRotationXSpinBox->setSingleStep(1.0);
    m_boneRotationXSpinBox->setWrapping(true);
    rotLayout->addWidget(rotXLabel);
    rotLayout->addWidget(m_boneRotationXSpinBox);

    QLabel* rotYLabel = new QLabel("Y:");
    m_boneRotationYSpinBox = new QDoubleSpinBox();
    m_boneRotationYSpinBox->setRange(-360.0, 360.0);
    m_boneRotationYSpinBox->setDecimals(2);
    m_boneRotationYSpinBox->setSingleStep(1.0);
    m_boneRotationYSpinBox->setWrapping(true);
    rotLayout->addWidget(rotYLabel);
    rotLayout->addWidget(m_boneRotationYSpinBox);

    QLabel* rotZLabel = new QLabel("Z:");
    m_boneRotationZSpinBox = new QDoubleSpinBox();
    m_boneRotationZSpinBox->setRange(-360.0, 360.0);
    m_boneRotationZSpinBox->setDecimals(2);
    m_boneRotationZSpinBox->setSingleStep(1.0);
    m_boneRotationZSpinBox->setWrapping(true);
    rotLayout->addWidget(rotZLabel);
    rotLayout->addWidget(m_boneRotationZSpinBox);

    transformLayout->addLayout(rotLayout);

    // Scale controls
    QLabel* scaleLabel = new QLabel("Scale:");
    transformLayout->addWidget(scaleLabel);
    QHBoxLayout* scaleLayout = new QHBoxLayout();

    QLabel* scaleXLabel = new QLabel("X:");
    m_boneScaleXSpinBox = new QDoubleSpinBox();
    m_boneScaleXSpinBox->setRange(0.001, 100.0);
    m_boneScaleXSpinBox->setDecimals(3);
    m_boneScaleXSpinBox->setSingleStep(0.1);
    m_boneScaleXSpinBox->setValue(1.0);
    scaleLayout->addWidget(scaleXLabel);
    scaleLayout->addWidget(m_boneScaleXSpinBox);

    QLabel* scaleYLabel = new QLabel("Y:");
    m_boneScaleYSpinBox = new QDoubleSpinBox();
    m_boneScaleYSpinBox->setRange(0.001, 100.0);
    m_boneScaleYSpinBox->setDecimals(3);
    m_boneScaleYSpinBox->setSingleStep(0.1);
    m_boneScaleYSpinBox->setValue(1.0);
    scaleLayout->addWidget(scaleYLabel);
    scaleLayout->addWidget(m_boneScaleYSpinBox);

    QLabel* scaleZLabel = new QLabel("Z:");
    m_boneScaleZSpinBox = new QDoubleSpinBox();
    m_boneScaleZSpinBox->setRange(0.001, 100.0);
    m_boneScaleZSpinBox->setDecimals(3);
    m_boneScaleZSpinBox->setSingleStep(0.1);
    m_boneScaleZSpinBox->setValue(1.0);
    scaleLayout->addWidget(scaleZLabel);
    scaleLayout->addWidget(m_boneScaleZSpinBox);

    transformLayout->addLayout(scaleLayout);

    // Keyframe controls
    QHBoxLayout* keyframeLayout = new QHBoxLayout();
    m_setKeyframeButton = new QPushButton("Set Keyframe");
    m_setKeyframeButton->setMinimumHeight(25);
    m_deleteKeyframeButton = new QPushButton("Delete Keyframe");
    m_deleteKeyframeButton->setMinimumHeight(25);
    keyframeLayout->addWidget(m_setKeyframeButton);
    keyframeLayout->addWidget(m_deleteKeyframeButton);
    transformLayout->addLayout(keyframeLayout);

    animationLayout->addWidget(m_boneTransformGroup);

    animLayout->addWidget(m_animationGroup);

    // Animation controls group
    m_animationControlsGroup = new QGroupBox("Animation Controls");
    m_animationControlsGroup->setMinimumHeight(200);
    QVBoxLayout* animControlsLayout = new QVBoxLayout(m_animationControlsGroup);
    animControlsLayout->setSpacing(6);

    // Animation management
    QHBoxLayout* animButtonLayout = new QHBoxLayout();
    m_createAnimationButton = new QPushButton("Create");
    m_createAnimationButton->setMinimumHeight(30);
    m_deleteAnimationButton = new QPushButton("Delete");
    m_deleteAnimationButton->setMinimumHeight(30);
    animButtonLayout->addWidget(m_createAnimationButton);
    animButtonLayout->addWidget(m_deleteAnimationButton);
    animControlsLayout->addLayout(animButtonLayout);

    // Animations list
    QLabel* animationsLabel = new QLabel("Animations:");
    animControlsLayout->addWidget(animationsLabel);
    m_animationsList = new QListWidget();
    m_animationsList->setMinimumHeight(60);
    m_animationsList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_animationsList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_animationsList, &QListWidget::customContextMenuRequested, this, &VoxelBlockerDialog::showAnimationContextMenu);
    animControlsLayout->addWidget(m_animationsList);

    // Playback controls
    QHBoxLayout* playbackLayout = new QHBoxLayout();
    m_playAnimationButton = new QPushButton("Play");
    m_playAnimationButton->setMinimumHeight(30);
    m_stopAnimationButton = new QPushButton("Stop");
    m_stopAnimationButton->setMinimumHeight(30);
    playbackLayout->addWidget(m_playAnimationButton);
    playbackLayout->addWidget(m_stopAnimationButton);
    animControlsLayout->addLayout(playbackLayout);

    // Animation time
    QLabel* timeLabel = new QLabel("Time:");
    animControlsLayout->addWidget(timeLabel);

    m_animationTimeSlider = new QSlider(Qt::Horizontal);
    m_animationTimeSlider->setRange(0, 1000);
    m_animationTimeSlider->setValue(0);
    animControlsLayout->addWidget(m_animationTimeSlider);

    QHBoxLayout* timeSpinLayout = new QHBoxLayout();
    m_animationTimeSpinBox = new QDoubleSpinBox();
    m_animationTimeSpinBox->setRange(0.0, 10.0);
    m_animationTimeSpinBox->setValue(0.0);
    m_animationTimeSpinBox->setDecimals(2);
    m_animationTimeSpinBox->setSingleStep(0.1);
    m_animationTimeSpinBox->setMaximumWidth(80);
    timeSpinLayout->addWidget(m_animationTimeSpinBox);

    QLabel* speedLabel = new QLabel("Speed:");
    timeSpinLayout->addWidget(speedLabel);
    m_animationSpeedSpinBox = new QDoubleSpinBox();
    m_animationSpeedSpinBox->setRange(0.1, 5.0);
    m_animationSpeedSpinBox->setValue(1.0);
    m_animationSpeedSpinBox->setDecimals(1);
    m_animationSpeedSpinBox->setSingleStep(0.1);
    m_animationSpeedSpinBox->setMaximumWidth(60);
    timeSpinLayout->addWidget(m_animationSpeedSpinBox);

    animControlsLayout->addLayout(timeSpinLayout);

    // Animation duration
    QHBoxLayout* durationLayout = new QHBoxLayout();
    QLabel* durationLabel = new QLabel("Duration:");
    durationLayout->addWidget(durationLabel);

    m_animationDurationSpinBox = new QDoubleSpinBox();
    m_animationDurationSpinBox->setRange(0.1, 60.0); // 0.1 to 60 seconds
    m_animationDurationSpinBox->setValue(1.0);
    m_animationDurationSpinBox->setDecimals(1);
    m_animationDurationSpinBox->setSingleStep(0.1);
    m_animationDurationSpinBox->setSuffix(" sec");
    m_animationDurationSpinBox->setMaximumWidth(100);
    durationLayout->addWidget(m_animationDurationSpinBox);

    durationLayout->addStretch();
    animControlsLayout->addLayout(durationLayout);

    // Animation status
    m_animationStatusLabel = new QLabel("No animation selected");
    m_animationStatusLabel->setStyleSheet("font-size: 10px; color: gray;");
    animControlsLayout->addWidget(m_animationStatusLabel);

    animLayout->addWidget(m_animationControlsGroup);

    // Advanced tools group
    m_advancedToolsGroup = new QGroupBox("Advanced Tools");
    m_advancedToolsGroup->setMinimumHeight(150);
    QVBoxLayout* advancedToolsLayout = new QVBoxLayout(m_advancedToolsGroup);
    advancedToolsLayout->setSpacing(6);

    // Symmetry mode
    QLabel* symmetryLabel = new QLabel("Symmetry Mode:");
    advancedToolsLayout->addWidget(symmetryLabel);

    m_symmetryModeCombo = new QComboBox();
    m_symmetryModeCombo->addItems({"None", "X-Axis", "Y-Axis", "Z-Axis", "XY-Plane", "XZ-Plane", "YZ-Plane", "All Axes"});
    advancedToolsLayout->addWidget(m_symmetryModeCombo);

    // Symmetry center
    QLabel* centerLabel = new QLabel("Symmetry Center:");
    advancedToolsLayout->addWidget(centerLabel);

    QHBoxLayout* centerLayout = new QHBoxLayout();
    m_symmetryCenterXSpinBox = new QDoubleSpinBox();
    m_symmetryCenterXSpinBox->setRange(-100.0, 100.0);
    m_symmetryCenterXSpinBox->setValue(0.0);
    m_symmetryCenterXSpinBox->setDecimals(1);
    m_symmetryCenterXSpinBox->setMaximumWidth(60);
    centerLayout->addWidget(new QLabel("X:"));
    centerLayout->addWidget(m_symmetryCenterXSpinBox);

    m_symmetryCenterYSpinBox = new QDoubleSpinBox();
    m_symmetryCenterYSpinBox->setRange(-100.0, 100.0);
    m_symmetryCenterYSpinBox->setValue(0.0);
    m_symmetryCenterYSpinBox->setDecimals(1);
    m_symmetryCenterYSpinBox->setMaximumWidth(60);
    centerLayout->addWidget(new QLabel("Y:"));
    centerLayout->addWidget(m_symmetryCenterYSpinBox);

    m_symmetryCenterZSpinBox = new QDoubleSpinBox();
    m_symmetryCenterZSpinBox->setRange(-100.0, 100.0);
    m_symmetryCenterZSpinBox->setValue(0.0);
    m_symmetryCenterZSpinBox->setDecimals(1);
    m_symmetryCenterZSpinBox->setMaximumWidth(60);
    centerLayout->addWidget(new QLabel("Z:"));
    centerLayout->addWidget(m_symmetryCenterZSpinBox);

    advancedToolsLayout->addLayout(centerLayout);

    // Add stretch to animation panel
    animLayout->addStretch();

    // Set up scroll area and dock
    m_animationScrollArea = new QScrollArea();
    m_animationScrollArea->setWidgetResizable(true);
    m_animationScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_animationScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_animationScrollArea->setWidget(m_animationPanel);
    m_animationDock->setWidget(m_animationScrollArea);

    // Connect animation panel signals
    connect(m_riggingModeCheck, &QCheckBox::toggled, this, &VoxelBlockerDialog::onRiggingModeChanged);
    connect(m_showBonesCheck, &QCheckBox::toggled, this, &VoxelBlockerDialog::onShowBonesChanged);
    connect(m_createBoneButton, &QPushButton::clicked, this, &VoxelBlockerDialog::onCreateBoneClicked);
    connect(m_deleteBoneButton, &QPushButton::clicked, this, &VoxelBlockerDialog::onDeleteBoneClicked);
    connect(m_assignBoneButton, &QPushButton::clicked, this, &VoxelBlockerDialog::onAssignBoneClicked);
    connect(m_bonesList, &QListWidget::itemSelectionChanged, this, &VoxelBlockerDialog::onBoneSelectionChanged);

    // Bone transform controls
    connect(m_bonePositionXSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VoxelBlockerDialog::onBoneTransformChanged);
    connect(m_bonePositionYSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VoxelBlockerDialog::onBoneTransformChanged);
    connect(m_bonePositionZSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VoxelBlockerDialog::onBoneTransformChanged);
    connect(m_boneRotationXSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VoxelBlockerDialog::onBoneTransformChanged);
    connect(m_boneRotationYSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VoxelBlockerDialog::onBoneTransformChanged);
    connect(m_boneRotationZSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VoxelBlockerDialog::onBoneTransformChanged);
    connect(m_boneScaleXSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VoxelBlockerDialog::onBoneTransformChanged);
    connect(m_boneScaleYSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VoxelBlockerDialog::onBoneTransformChanged);
    connect(m_boneScaleZSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VoxelBlockerDialog::onBoneTransformChanged);
    connect(m_setKeyframeButton, &QPushButton::clicked, this, &VoxelBlockerDialog::onSetKeyframeClicked);
    connect(m_deleteKeyframeButton, &QPushButton::clicked, this, &VoxelBlockerDialog::onDeleteKeyframeClicked);

    // Animation controls
    connect(m_createAnimationButton, &QPushButton::clicked, this, &VoxelBlockerDialog::onCreateAnimationClicked);
    connect(m_deleteAnimationButton, &QPushButton::clicked, this, &VoxelBlockerDialog::onDeleteAnimationClicked);
    connect(m_playAnimationButton, &QPushButton::clicked, this, &VoxelBlockerDialog::onPlayAnimationClicked);
    connect(m_stopAnimationButton, &QPushButton::clicked, this, &VoxelBlockerDialog::onStopAnimationClicked);
    connect(m_animationsList, &QListWidget::itemSelectionChanged, this, &VoxelBlockerDialog::onAnimationSelectionChanged);
    connect(m_animationTimeSlider, &QSlider::valueChanged, this, &VoxelBlockerDialog::onAnimationTimeChanged);
    connect(m_animationTimeSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VoxelBlockerDialog::onAnimationTimeChanged);
    connect(m_animationSpeedSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VoxelBlockerDialog::onAnimationSpeedChanged);
    connect(m_animationDurationSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VoxelBlockerDialog::onAnimationDurationChanged);
}

void VoxelBlockerDialog::setupTimelinePanel() {
    // Create timeline panel widget
    m_timelinePanel = new QWidget();
    QVBoxLayout* timelineLayout = new QVBoxLayout(m_timelinePanel);
    timelineLayout->setSpacing(8);
    timelineLayout->setContentsMargins(8, 8, 8, 8);

    // Timeline controls
    QHBoxLayout* timelineControlsLayout = new QHBoxLayout();
    m_addKeyframeButton = new QPushButton("Add Keyframe");
    m_removeKeyframeButton = new QPushButton("Remove Keyframe");
    m_addKeyframeButton->setMinimumHeight(30);
    m_removeKeyframeButton->setMinimumHeight(30);
    timelineControlsLayout->addWidget(m_addKeyframeButton);
    timelineControlsLayout->addWidget(m_removeKeyframeButton);
    timelineControlsLayout->addStretch();
    timelineLayout->addLayout(timelineControlsLayout);

    // Timeline tree
    m_timelineTree = new QTreeWidget();
    m_timelineTree->setHeaderLabels({"Bone", "Time", "Value"});
    m_timelineTree->setMinimumHeight(150);
    timelineLayout->addWidget(m_timelineTree);

    // Timeline slider
    QHBoxLayout* sliderLayout = new QHBoxLayout();
    m_timelineLabel = new QLabel("Time: 0.00s");
    sliderLayout->addWidget(m_timelineLabel);

    m_timelineSlider = new QSlider(Qt::Horizontal);
    m_timelineSlider->setRange(0, 1000);
    m_timelineSlider->setValue(0);
    sliderLayout->addWidget(m_timelineSlider);
    timelineLayout->addLayout(sliderLayout);

    // Set up dock
    m_timelineDock->setWidget(m_timelinePanel);

    // Connect timeline signals
    connect(m_addKeyframeButton, &QPushButton::clicked, this, &VoxelBlockerDialog::onAddKeyframeClicked);
    connect(m_removeKeyframeButton, &QPushButton::clicked, this, &VoxelBlockerDialog::onRemoveKeyframeClicked);
    connect(m_timelineTree, &QTreeWidget::itemSelectionChanged, this, &VoxelBlockerDialog::onTimelineSelectionChanged);
    connect(m_timelineTree, &QTreeWidget::itemDoubleClicked, this, &VoxelBlockerDialog::onKeyframeDoubleClicked);
    connect(m_timelineSlider, &QSlider::valueChanged, this, [this](int value) {
        float time = value / 1000.0f;
        m_timelineLabel->setText(QString("Time: %1s").arg(time, 0, 'f', 2));
    });
}

void VoxelBlockerDialog::setupViewportPanel() {
    // Create central widget (viewport)
    m_centralWidget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(m_centralWidget);
    layout->setContentsMargins(0, 0, 0, 0);

    m_canvas = new VoxelCanvas(m_centralWidget);
    layout->addWidget(m_canvas);

    // Set as central widget
    setCentralWidget(m_centralWidget);
}

// Slot implementations
void VoxelBlockerDialog::onNewScene() {
    if (hasUnsavedChanges() && !promptSaveChanges()) {
        return;
    }

    if (m_canvas) {
        m_canvas->NewScene();
    }

    m_currentFilePath.clear();
    setModified(false);
    updateWindowTitle();
    updateVoxelCount();
}

void VoxelBlockerDialog::onOpenFile() {
    if (hasUnsavedChanges() && !promptSaveChanges()) {
        return;
    }

    QString filepath = QFileDialog::getOpenFileName(
        this,
        "Open Voxel Scene",
        QDir::currentPath(),
        "Voxel Files (*.voxels);;All Files (*)"
    );

    if (!filepath.isEmpty() && m_canvas) {
        if (m_canvas->LoadFromFile(filepath)) {
            m_currentFilePath = filepath;
            setModified(false);
            updateWindowTitle();
            updateVoxelCount();
        } else {
            QMessageBox::warning(this, "Error", "Failed to load file.");
        }
    }
}

void VoxelBlockerDialog::onSaveFile() {
    if (m_currentFilePath.isEmpty()) {
        onSaveAs();
    } else if (m_canvas) {
        if (m_canvas->SaveToFile(m_currentFilePath)) {
            setModified(false);
            updateWindowTitle();
        } else {
            QMessageBox::warning(this, "Error", "Failed to save file.");
        }
    }
}

void VoxelBlockerDialog::onSaveAs() {
    QString filepath = QFileDialog::getSaveFileName(
        this,
        "Save Voxel Scene",
        QDir::currentPath(),
        "Voxel Files (*.voxels);;All Files (*)"
    );

    if (!filepath.isEmpty() && m_canvas) {
        if (m_canvas->SaveToFile(filepath)) {
            m_currentFilePath = filepath;
            setModified(false);
            updateWindowTitle();
        } else {
            QMessageBox::warning(this, "Error", "Failed to save file.");
        }
    }
}

void VoxelBlockerDialog::onExportOBJ() {
    // Create export options dialog
    QDialog optionsDialog(this);
    optionsDialog.setWindowTitle("OBJ Export Options");
    optionsDialog.setModal(true);
    optionsDialog.resize(300, 150);

    QVBoxLayout* layout = new QVBoxLayout(&optionsDialog);

    // Face merging option
    QCheckBox* mergeFacesCheck = new QCheckBox("Merge external faces (optimize mesh)");
    mergeFacesCheck->setChecked(false);
    mergeFacesCheck->setToolTip("Combines adjacent external faces and removes internal faces to reduce polygon count");
    layout->addWidget(mergeFacesCheck);

    // Texture atlas option
    QCheckBox* textureAtlasCheck = new QCheckBox("Use texture atlas for colors (recommended)");
    textureAtlasCheck->setChecked(true);
    textureAtlasCheck->setToolTip("Creates a texture atlas PNG file and uses UV coordinates instead of vertex colors for better compatibility");
    layout->addWidget(textureAtlasCheck);

    // Info label
    QLabel* infoLabel = new QLabel("Texture atlas provides better compatibility with 3D software than vertex colors.");
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("color: gray; font-size: 10px;");
    layout->addWidget(infoLabel);

    layout->addStretch();

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* exportButton = new QPushButton("Export");
    QPushButton* cancelButton = new QPushButton("Cancel");

    buttonLayout->addStretch();
    buttonLayout->addWidget(exportButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);

    connect(exportButton, &QPushButton::clicked, &optionsDialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &optionsDialog, &QDialog::reject);

    if (optionsDialog.exec() == QDialog::Accepted) {
        QString filepath = QFileDialog::getSaveFileName(
            this,
            "Export to OBJ",
            QDir::currentPath(),
            "OBJ Files (*.obj);;All Files (*)"
        );

        if (!filepath.isEmpty() && m_canvas) {
            bool mergeFaces = mergeFacesCheck->isChecked();
            bool useTextureAtlas = textureAtlasCheck->isChecked();
            if (!m_canvas->ExportToOBJ(filepath, mergeFaces, useTextureAtlas)) {
                QMessageBox::warning(this, "Error", "Failed to export OBJ file.");
            } else {
                QString message = QString("Successfully exported to %1\n\nFace merging: %2\nTexture atlas: %3")
                    .arg(QFileInfo(filepath).fileName())
                    .arg(mergeFaces ? "Enabled" : "Disabled")
                    .arg(useTextureAtlas ? "Enabled" : "Disabled");

                if (useTextureAtlas) {
                    QString textureName = QFileInfo(filepath).baseName() + "_atlas.png";
                    message += "\n\nGenerated files:\n- " + QFileInfo(filepath).fileName();
                    message += "\n- " + textureName;
                    message += "\n- " + QFileInfo(filepath).baseName() + ".mtl";
                }

                QMessageBox::information(this, "Export Complete", message);
            }
        }
    }
}

void VoxelBlockerDialog::onExportFBX() {
    // Create export options dialog
    QDialog optionsDialog(this);
    optionsDialog.setWindowTitle("FBX Export Options");
    optionsDialog.setModal(true);
    optionsDialog.resize(300, 150);

    QVBoxLayout* layout = new QVBoxLayout(&optionsDialog);

    // Texture atlas option
    QCheckBox* textureAtlasCheck = new QCheckBox("Use texture atlas for colors (recommended)");
    textureAtlasCheck->setChecked(true);
    textureAtlasCheck->setToolTip("Creates a texture atlas PNG file and uses UV coordinates instead of vertex colors for better compatibility");
    layout->addWidget(textureAtlasCheck);

    // Info label
    QLabel* infoLabel = new QLabel("FBX export includes mesh geometry, texture atlas/vertex colors, bone rigging, and animations.");
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("color: #4CAF50; font-size: 10px;");
    layout->addWidget(infoLabel);

    layout->addStretch();

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* exportButton = new QPushButton("Export");
    QPushButton* cancelButton = new QPushButton("Cancel");

    buttonLayout->addStretch();
    buttonLayout->addWidget(exportButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);

    connect(exportButton, &QPushButton::clicked, &optionsDialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &optionsDialog, &QDialog::reject);

    if (optionsDialog.exec() == QDialog::Accepted) {
        QString filepath = QFileDialog::getSaveFileName(
            this,
            "Export to FBX",
            QDir::currentPath(),
            "FBX Files (*.fbx);;All Files (*)"
        );

        if (!filepath.isEmpty() && m_canvas) {
            bool useTextureAtlas = textureAtlasCheck->isChecked();
            if (!m_canvas->ExportToFBX(filepath, useTextureAtlas)) {
                QMessageBox::warning(this, "Error", "Failed to export FBX file.");
            } else {
                QString message = QString("Successfully exported to %1").arg(QFileInfo(filepath).fileName());
                if (useTextureAtlas) {
                    message += "\nTexture atlas saved as: " + QFileInfo(filepath).baseName() + "_atlas.png";
                }
                QMessageBox::information(this, "Export Complete", message);
            }
        }
    }
}

// Edit operations
void VoxelBlockerDialog::onUndo() {
    if (m_canvas) {
        m_canvas->Undo();
        updateUndoRedoActions();
    }
}

void VoxelBlockerDialog::onRedo() {
    if (m_canvas) {
        m_canvas->Redo();
        updateUndoRedoActions();
    }
}

void VoxelBlockerDialog::onCopy() {
    if (m_canvas) {
        m_canvas->CopySelectedVoxels(true, false);
    }
}

void VoxelBlockerDialog::onPaste() {
    if (m_canvas) {
        m_canvas->Paste(glm::vec3(0.0f), true, false);
    }
}

void VoxelBlockerDialog::onCut() {
    if (m_canvas) {
        m_canvas->CopySelectedVoxels(true, false);
        // Remove selected voxels after copying
        m_canvas->DeleteSelectedVoxels();
    }
}

void VoxelBlockerDialog::onVoxelSizeChanged() {
    float size = 1.0f;

    QObject* sender = this->sender();
    if (sender == m_voxelSizeSlider) {
        size = m_voxelSizeSlider->value() / 10.0f;
        m_voxelSizeSpinBox->blockSignals(true);
        m_voxelSizeSpinBox->setValue(size);
        m_voxelSizeSpinBox->blockSignals(false);
    } else if (sender == m_voxelSizeSpinBox) {
        size = static_cast<float>(m_voxelSizeSpinBox->value());
        m_voxelSizeSlider->blockSignals(true);
        m_voxelSizeSlider->setValue(static_cast<int>(size * 10));
        m_voxelSizeSlider->blockSignals(false);
    }

    if (m_canvas) {
        m_canvas->SetVoxelSize(size);
    }
}

void VoxelBlockerDialog::onVoxelColorChanged() {
    QColor color = QColorDialog::getColor(m_canvas ? m_canvas->GetVoxelColor() : Qt::red, this);
    if (color.isValid() && m_canvas) {
        m_canvas->SetVoxelColor(color);
        m_voxelColorButton->setStyleSheet(QString("background-color: %1; border: 2px solid gray;").arg(color.name()));
    }
}

void VoxelBlockerDialog::onPlacementModeChanged() {
    if (m_canvas) {
        VoxelPlacementMode mode = static_cast<VoxelPlacementMode>(m_placementModeCombo->currentIndex());
        m_canvas->SetPlacementMode(mode);
    }
}

void VoxelBlockerDialog::onGridSizeChanged() {
    float size = 1.0f;

    QObject* sender = this->sender();
    if (sender == m_gridSizeSlider) {
        size = m_gridSizeSlider->value() / 10.0f;
        m_gridSizeSpinBox->blockSignals(true);
        m_gridSizeSpinBox->setValue(size);
        m_gridSizeSpinBox->blockSignals(false);
    } else if (sender == m_gridSizeSpinBox) {
        size = static_cast<float>(m_gridSizeSpinBox->value());
        m_gridSizeSlider->blockSignals(true);
        m_gridSizeSlider->setValue(static_cast<int>(size * 10));
        m_gridSizeSlider->blockSignals(false);
    }

    if (m_canvas) {
        m_canvas->SetGridSize(size);
    }
}

void VoxelBlockerDialog::onShowGridChanged() {
    if (m_canvas) {
        m_canvas->SetShowGrid(m_showGridCheck->isChecked());
    }
}

void VoxelBlockerDialog::onResetCamera() {
    if (m_canvas) {
        m_canvas->ResetCamera();
    }
}

void VoxelBlockerDialog::onFocusOnVoxels() {
    if (m_canvas) {
        m_canvas->FocusOnVoxels();
    }
}

void VoxelBlockerDialog::onVoxelAdded(const glm::vec3& position) {
    Q_UNUSED(position)
    m_voxelCount++;
    updateVoxelCount();
    setModified(true);
}

void VoxelBlockerDialog::onVoxelRemoved(const glm::vec3& position) {
    Q_UNUSED(position)
    m_voxelCount--;
    updateVoxelCount();
    setModified(true);
}

void VoxelBlockerDialog::onSceneModified() {
    setModified(true);
    updateUndoRedoActions();
}

// Tool selection slots
void VoxelBlockerDialog::onToolChanged(int toolId) {
    VoxelTool tool = static_cast<VoxelTool>(toolId);
    if (m_canvas) {
        m_canvas->SetCurrentTool(tool);
    }

    // Update UI based on selected tool
    switch (tool) {
        case VoxelTool::Place:
            // Enable placement mode controls
            m_placementModeCombo->setEnabled(true);
            m_gizmoGroup->setVisible(false);
            break;
        case VoxelTool::Select:
            // Disable placement mode for selection
            m_placementModeCombo->setEnabled(false);
            m_gizmoGroup->setVisible(true);
            break;
        default:
            m_placementModeCombo->setEnabled(true);
            m_gizmoGroup->setVisible(false);
            break;
    }
}

void VoxelBlockerDialog::onPlaceToolSelected() {
    m_placeToolButton->setChecked(true);
    onToolChanged(static_cast<int>(VoxelTool::Place));
}

void VoxelBlockerDialog::onEraseToolSelected() {
    m_eraseToolButton->setChecked(true);
    onToolChanged(static_cast<int>(VoxelTool::Erase));
}

void VoxelBlockerDialog::onSelectToolSelected() {
    m_selectToolButton->setChecked(true);
    onToolChanged(static_cast<int>(VoxelTool::Select));
}

void VoxelBlockerDialog::onFloodFillToolSelected() {
    m_floodFillToolButton->setChecked(true);
    onToolChanged(static_cast<int>(VoxelTool::FloodFill));
}

void VoxelBlockerDialog::onLineToolSelected() {
    m_lineToolButton->setChecked(true);
    onToolChanged(static_cast<int>(VoxelTool::Line));
}

void VoxelBlockerDialog::onRectangleToolSelected() {
    m_rectangleToolButton->setChecked(true);
    onToolChanged(static_cast<int>(VoxelTool::Rectangle));
}

void VoxelBlockerDialog::onSphereToolSelected() {
    m_sphereToolButton->setChecked(true);
    onToolChanged(static_cast<int>(VoxelTool::Sphere));
}

void VoxelBlockerDialog::onGizmoModeChanged(GizmoMode mode) {
    if (m_canvas) {
        m_canvas->SetGizmoMode(mode);
    }
}

// Grid control slots
void VoxelBlockerDialog::onGridBaseYChanged() {
    if (m_canvas) {
        float baseY = m_gridBaseYSpinBox->value();
        m_canvas->SetGridBaseY(baseY);
    }
}

void VoxelBlockerDialog::onGridUpClicked() {
    if (m_canvas) {
        float currentY = m_gridBaseYSpinBox->value();
        float voxelSize = m_canvas->GetVoxelSize();
        float newY = currentY + voxelSize;

        // Update both UI and canvas
        m_gridBaseYSpinBox->blockSignals(true);
        m_gridBaseYSpinBox->setValue(newY);
        m_gridBaseYSpinBox->blockSignals(false);

        m_canvas->SetGridBaseY(newY);

        // Force update
        m_canvas->update();
    }
}

void VoxelBlockerDialog::onGridDownClicked() {
    if (m_canvas) {
        float currentY = m_gridBaseYSpinBox->value();
        float voxelSize = m_canvas->GetVoxelSize();
        float newY = currentY - voxelSize;

        // Update both UI and canvas
        m_gridBaseYSpinBox->blockSignals(true);
        m_gridBaseYSpinBox->setValue(newY);
        m_gridBaseYSpinBox->blockSignals(false);

        m_canvas->SetGridBaseY(newY);

        // Force update
        m_canvas->update();
    }
}

// Face cycling slot
void VoxelBlockerDialog::onFaceChanged(int face) {
    QString faceNames[] = {
        "+X (Right)", "-X (Left)", "+Y (Up)",
        "-Y (Down)", "+Z (Forward)", "-Z (Back)"
    };

    QString faceColors[] = {
        "#FF6B6B", "#4ECDC4", "#45B7D1",
        "#96CEB4", "#FFEAA7", "#DDA0DD"
    };

    if (face >= 0 && face < 6) {
        m_faceDisplayLabel->setText(faceNames[face]);
        m_faceDisplayLabel->setStyleSheet(QString("font-weight: bold; color: %1; font-size: 12px;").arg(faceColors[face]));

        // Update tooltip with more information
        QString tooltip = QString("Current face: %1\nUse arrow keys to cycle faces\nMost useful in Face Snap mode").arg(faceNames[face]);
        m_faceDisplayLabel->setToolTip(tooltip);
    }
}

// Animation system slots
void VoxelBlockerDialog::onRiggingModeChanged(bool enabled) {
    if (m_canvas) {
        m_canvas->SetRiggingMode(enabled);
    }
}

void VoxelBlockerDialog::onShowBonesChanged(bool show) {
    if (m_canvas) {
        m_canvas->SetShowBones(show);
    }
}

void VoxelBlockerDialog::onCreateBoneClicked() {
    if (m_canvas) {
        QString name = QString("Bone_%1").arg(m_bonesList->count() + 1);
        glm::vec3 position(0.0f, 0.0f, 0.0f);
        int boneId = m_canvas->CreateBone(name, position);

        // Add to UI list
        QListWidgetItem* item = new QListWidgetItem(name);
        item->setData(Qt::UserRole, boneId);
        m_bonesList->addItem(item);
    }
}

void VoxelBlockerDialog::onDeleteBoneClicked() {
    QListWidgetItem* currentItem = m_bonesList->currentItem();
    if (currentItem && m_canvas) {
        int boneId = currentItem->data(Qt::UserRole).toInt();
        m_canvas->DeleteBone(boneId);

        // Remove from UI list
        delete currentItem;
    }
}

void VoxelBlockerDialog::onAssignBoneClicked() {
    QListWidgetItem* currentItem = m_bonesList->currentItem();
    if (currentItem && m_canvas) {
        int boneId = currentItem->data(Qt::UserRole).toInt();

        // Assign all selected voxels to this bone
        std::vector<Voxel*> selectedVoxels = m_canvas->GetSelectedVoxels();
        for (Voxel* voxel : selectedVoxels) {
            m_canvas->AssignVoxelToBone(voxel->position, boneId);
        }

        // Update the display
        m_canvas->update();

        // Show feedback
        QString boneName = currentItem->text();
        QString message = QString("Assigned %1 voxel(s) to bone '%2'").arg(selectedVoxels.size()).arg(boneName);
        // Could add a status message here
    }
}

void VoxelBlockerDialog::onBoneSelectionChanged() {
    QListWidgetItem* currentItem = m_bonesList->currentItem();
    if (currentItem && m_canvas) {
        int boneId = currentItem->data(Qt::UserRole).toInt();
        m_canvas->SelectBone(boneId);

        // Enable assign button if bone is selected and voxels are selected
        bool hasSelectedVoxels = m_canvas->GetSelectedVoxelCount() > 0;
        m_assignBoneButton->setEnabled(hasSelectedVoxels);

        // Update bone transform UI with selected bone's values
        updateBoneTransformUI(boneId);
        m_boneTransformGroup->setEnabled(true);
    } else {
        m_assignBoneButton->setEnabled(false);
        m_boneTransformGroup->setEnabled(false);
    }
}

void VoxelBlockerDialog::updateBoneTransformUI(int boneId) {
    if (!m_canvas) return;

    VoxelBone* bone = m_canvas->GetBone(boneId);
    if (!bone) return;

    // Block signals to prevent recursive updates
    m_bonePositionXSpinBox->blockSignals(true);
    m_bonePositionYSpinBox->blockSignals(true);
    m_bonePositionZSpinBox->blockSignals(true);
    m_boneRotationXSpinBox->blockSignals(true);
    m_boneRotationYSpinBox->blockSignals(true);
    m_boneRotationZSpinBox->blockSignals(true);
    m_boneScaleXSpinBox->blockSignals(true);
    m_boneScaleYSpinBox->blockSignals(true);
    m_boneScaleZSpinBox->blockSignals(true);

    // Update position
    m_bonePositionXSpinBox->setValue(bone->position.x);
    m_bonePositionYSpinBox->setValue(bone->position.y);
    m_bonePositionZSpinBox->setValue(bone->position.z);

    // Convert quaternion to Euler angles for UI display
    glm::vec3 eulerAngles = glm::degrees(glm::eulerAngles(bone->rotation));
    m_boneRotationXSpinBox->setValue(eulerAngles.x);
    m_boneRotationYSpinBox->setValue(eulerAngles.y);
    m_boneRotationZSpinBox->setValue(eulerAngles.z);

    // Update scale
    m_boneScaleXSpinBox->setValue(bone->scale.x);
    m_boneScaleYSpinBox->setValue(bone->scale.y);
    m_boneScaleZSpinBox->setValue(bone->scale.z);

    // Unblock signals
    m_bonePositionXSpinBox->blockSignals(false);
    m_bonePositionYSpinBox->blockSignals(false);
    m_bonePositionZSpinBox->blockSignals(false);
    m_boneRotationXSpinBox->blockSignals(false);
    m_boneRotationYSpinBox->blockSignals(false);
    m_boneRotationZSpinBox->blockSignals(false);
    m_boneScaleXSpinBox->blockSignals(false);
    m_boneScaleYSpinBox->blockSignals(false);
    m_boneScaleZSpinBox->blockSignals(false);
}

void VoxelBlockerDialog::onBoneTransformChanged() {
    if (!m_canvas) return;

    QListWidgetItem* currentItem = m_bonesList->currentItem();
    if (!currentItem) return;

    int boneId = currentItem->data(Qt::UserRole).toInt();
    VoxelBone* bone = m_canvas->GetBone(boneId);
    if (!bone) return;

    // Update bone transform from UI values
    bone->position.x = m_bonePositionXSpinBox->value();
    bone->position.y = m_bonePositionYSpinBox->value();
    bone->position.z = m_bonePositionZSpinBox->value();

    // Convert Euler angles to quaternion
    glm::vec3 eulerRadians = glm::radians(glm::vec3(
        m_boneRotationXSpinBox->value(),
        m_boneRotationYSpinBox->value(),
        m_boneRotationZSpinBox->value()
    ));
    bone->rotation = glm::quat(eulerRadians);

    bone->scale.x = m_boneScaleXSpinBox->value();
    bone->scale.y = m_boneScaleYSpinBox->value();
    bone->scale.z = m_boneScaleZSpinBox->value();

    // Update assigned voxels based on bone transform
    m_canvas->UpdateBoneTransforms();
    m_canvas->update();
}

void VoxelBlockerDialog::onSetKeyframeClicked() {
    if (!m_canvas) return;

    QListWidgetItem* currentItem = m_bonesList->currentItem();
    if (!currentItem) return;

    int boneId = currentItem->data(Qt::UserRole).toInt();

    // Get current animation time
    float currentTime = m_canvas->GetCurrentAnimationTime();

    // Set keyframe for current bone at current time
    m_canvas->SetBoneKeyframe(boneId, currentTime);

    // Update UI to reflect keyframe was set
    // Could add visual feedback here
}

void VoxelBlockerDialog::onDeleteKeyframeClicked() {
    if (!m_canvas) return;

    QListWidgetItem* currentItem = m_bonesList->currentItem();
    if (!currentItem) return;

    int boneId = currentItem->data(Qt::UserRole).toInt();

    // Get current animation time
    float currentTime = m_canvas->GetCurrentAnimationTime();

    // Delete keyframe for current bone at current time
    m_canvas->DeleteBoneKeyframe(boneId, currentTime);
}

// Animation controls slots
void VoxelBlockerDialog::onCreateAnimationClicked() {
    if (m_canvas) {
        QString name = QString("Animation_%1").arg(m_animationsList->count() + 1);
        float duration = 1.0f;
        int animId = m_canvas->CreateAnimation(name, duration);

        // Add to UI list
        QListWidgetItem* item = new QListWidgetItem(name);
        item->setData(Qt::UserRole, animId);
        m_animationsList->addItem(item);
    }
}

void VoxelBlockerDialog::onDeleteAnimationClicked() {
    QListWidgetItem* currentItem = m_animationsList->currentItem();
    if (currentItem && m_canvas) {
        int animId = currentItem->data(Qt::UserRole).toInt();
        m_canvas->DeleteAnimation(animId);

        // Remove from UI list
        delete currentItem;

        // Update status
        m_animationStatusLabel->setText("No animation selected");
    }
}

void VoxelBlockerDialog::onPlayAnimationClicked() {
    QListWidgetItem* currentItem = m_animationsList->currentItem();
    if (currentItem && m_canvas) {
        int animId = currentItem->data(Qt::UserRole).toInt();
        m_canvas->PlayAnimation(animId);
        m_animationStatusLabel->setText("Playing: " + currentItem->text());
    }
}

void VoxelBlockerDialog::onStopAnimationClicked() {
    if (m_canvas) {
        m_canvas->StopAnimation();
        m_animationStatusLabel->setText("Stopped");
    }
}

void VoxelBlockerDialog::onAnimationSelectionChanged() {
    QListWidgetItem* currentItem = m_animationsList->currentItem();
    if (currentItem && m_canvas) {
        int animId = currentItem->data(Qt::UserRole).toInt();
        m_canvas->SelectAnimation(animId);

        // Update time slider range based on animation duration
        float duration = m_canvas->GetAnimationDuration(animId);
        m_animationTimeSlider->setRange(0, static_cast<int>(duration * 1000));
        m_animationTimeSpinBox->setRange(0.0, duration);

        // Update timeline slider range
        m_timelineSlider->setRange(0, static_cast<int>(duration * 1000));

        // Refresh timeline to show keyframes for this animation
        refreshTimeline();

        m_animationStatusLabel->setText("Selected: " + currentItem->text());
    } else {
        m_animationStatusLabel->setText("No animation selected");
        m_timelineTree->clear();
    }
}

void VoxelBlockerDialog::onAnimationTimeChanged() {
    if (m_canvas) {
        float time = m_animationTimeSpinBox->value();

        // Update slider if change came from spinbox
        if (sender() == m_animationTimeSpinBox) {
            m_animationTimeSlider->blockSignals(true);
            m_animationTimeSlider->setValue(static_cast<int>(time * 1000));
            m_animationTimeSlider->blockSignals(false);
        }
        // Update spinbox if change came from slider
        else if (sender() == m_animationTimeSlider) {
            time = m_animationTimeSlider->value() / 1000.0f;
            m_animationTimeSpinBox->blockSignals(true);
            m_animationTimeSpinBox->setValue(time);
            m_animationTimeSpinBox->blockSignals(false);
        }

        m_canvas->SetAnimationTime(time);
    }
}

void VoxelBlockerDialog::onAnimationSpeedChanged() {
    if (m_canvas) {
        float speed = m_animationSpeedSpinBox->value();
        m_canvas->SetAnimationSpeed(speed);
    }
}

void VoxelBlockerDialog::onAnimationDurationChanged() {
    QListWidgetItem* currentItem = m_animationsList->currentItem();
    if (currentItem && m_canvas) {
        int animId = currentItem->data(Qt::UserRole).toInt();
        float duration = m_animationDurationSpinBox->value();

        // Update animation duration
        m_canvas->SetAnimationDuration(animId, duration);

        // Update time slider range
        m_animationTimeSlider->setRange(0, static_cast<int>(duration * 1000));
        m_animationTimeSpinBox->setRange(0.0, duration);
    }
}

// Advanced tools slots
void VoxelBlockerDialog::onSymmetryModeChanged() {
    if (m_canvas) {
        SymmetryMode mode = static_cast<SymmetryMode>(m_symmetryModeCombo->currentIndex());
        m_canvas->SetSymmetryMode(mode);
    }
}

void VoxelBlockerDialog::onSymmetryCenterChanged() {
    if (m_canvas) {
        glm::vec3 center(
            m_symmetryCenterXSpinBox->value(),
            m_symmetryCenterYSpinBox->value(),
            m_symmetryCenterZSpinBox->value()
        );
        m_canvas->SetSymmetryCenter(center);
    }
}



// Helper method for setting voxel color
void VoxelBlockerDialog::setVoxelColor(const QColor& color) {
    if (m_canvas) {
        m_canvas->SetVoxelColor(color);

        // Update color button display
        m_voxelColorButton->setStyleSheet(QString("background-color: %1; border: 2px solid gray;").arg(color.name()));
    }
}

// Utility methods
void VoxelBlockerDialog::updateWindowTitle() {
    QString title = "Voxel Blocker";
    if (!m_currentFilePath.isEmpty()) {
        QFileInfo fileInfo(m_currentFilePath);
        title += " - " + fileInfo.baseName();
    }
    if (m_modified) {
        title += " *";
    }
    setWindowTitle(title);
}

void VoxelBlockerDialog::updateVoxelCount() {
    if (m_voxelCountLabel && m_canvas) {
        int totalVoxels = m_canvas->GetVoxels().size();
        int selectedVoxels = m_canvas->GetSelectedVoxelCount();

        QString text = QString("Voxels: %1").arg(totalVoxels);
        if (selectedVoxels > 0) {
            text += QString(" (%1 selected)").arg(selectedVoxels);
        }

        m_voxelCountLabel->setText(text);
    }
}

void VoxelBlockerDialog::updateUndoRedoActions() {
    if (m_canvas && m_undoAction && m_redoAction) {
        bool canUndo = m_canvas->CanUndo();
        bool canRedo = m_canvas->CanRedo();

        m_undoAction->setEnabled(canUndo);
        m_redoAction->setEnabled(canRedo);

        if (canUndo) {
            QString undoText = m_canvas->GetUndoDescription();
            m_undoAction->setText(QString("&Undo %1").arg(undoText));
        } else {
            m_undoAction->setText("&Undo");
        }

        if (canRedo) {
            QString redoText = m_canvas->GetRedoDescription();
            m_redoAction->setText(QString("&Redo %1").arg(redoText));
        } else {
            m_redoAction->setText("&Redo");
        }
    }
}

bool VoxelBlockerDialog::hasUnsavedChanges() const {
    return m_modified;
}

bool VoxelBlockerDialog::promptSaveChanges() {
    if (!hasUnsavedChanges()) return true;

    QMessageBox::StandardButton result = QMessageBox::question(
        const_cast<VoxelBlockerDialog*>(this),
        "Unsaved Changes",
        "You have unsaved changes. Do you want to save them?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
    );

    switch (result) {
        case QMessageBox::Save:
            onSaveFile();
            return !hasUnsavedChanges(); // Return true if save was successful
        case QMessageBox::Discard:
            return true;
        case QMessageBox::Cancel:
        default:
            return false;
    }
}

void VoxelBlockerDialog::setModified(bool modified) {
    m_modified = modified;
    updateWindowTitle();
}

void VoxelBlockerDialog::refreshTimeline() {
    if (!m_canvas) {
        return;
    }

    // Clear existing timeline items
    m_timelineTree->clear();

    // Get currently selected animation
    QListWidgetItem* animItem = m_animationsList->currentItem();
    if (!animItem) {
        return;
    }

    int animationId = animItem->data(Qt::UserRole).toInt();
    VoxelAnimation* animation = m_canvas->GetSelectedAnimation();
    if (!animation) {
        return;
    }

    // Populate timeline with keyframes from all bones
    for (const auto& track : animation->tracks) {
        VoxelBone* bone = m_canvas->GetBone(track.boneId);
        if (!bone) continue;

        for (const auto& keyframe : track.keyframes) {
            QTreeWidgetItem* item = new QTreeWidgetItem(m_timelineTree);
            item->setText(0, bone->name);
            item->setText(1, QString::number(keyframe.time, 'f', 2));

            // Create detailed transform description
            QString transformDesc = QString("P(%.2f,%.2f,%.2f) R(%.1f,%.1f,%.1f) S(%.2f,%.2f,%.2f)")
                .arg(keyframe.position.x).arg(keyframe.position.y).arg(keyframe.position.z)
                .arg(glm::degrees(glm::eulerAngles(keyframe.rotation).x))
                .arg(glm::degrees(glm::eulerAngles(keyframe.rotation).y))
                .arg(glm::degrees(glm::eulerAngles(keyframe.rotation).z))
                .arg(keyframe.scale.x).arg(keyframe.scale.y).arg(keyframe.scale.z);

            item->setText(2, transformDesc);

            // Store keyframe data for easy access
            item->setData(0, Qt::UserRole, track.boneId);
            item->setData(1, Qt::UserRole, animationId);
            item->setData(2, Qt::UserRole, keyframe.time);

            m_timelineTree->addTopLevelItem(item);
        }
    }

    // Sort timeline items by time
    m_timelineTree->sortItems(1, Qt::AscendingOrder);
}

// Timeline control slots
void VoxelBlockerDialog::onAddKeyframeClicked() {
    // Add keyframe for currently selected bone at current time
    if (!m_canvas || m_bonesList->currentRow() < 0 || m_animationsList->currentRow() < 0) {
        return;
    }

    QListWidgetItem* boneItem = m_bonesList->currentItem();
    QListWidgetItem* animItem = m_animationsList->currentItem();
    if (!boneItem || !animItem) {
        return;
    }

    int boneId = boneItem->data(Qt::UserRole).toInt();
    int animationId = animItem->data(Qt::UserRole).toInt();
    float currentTime = m_timelineSlider->value() / 1000.0f;

    // Get the bone's current transform state
    VoxelBone* bone = m_canvas->GetBone(boneId);
    if (!bone) {
        return;
    }

    // Add keyframe to animation system with full transform state
    m_canvas->AddKeyframe(animationId, boneId, currentTime, bone->position, bone->rotation, bone->scale);

    // Add to timeline tree with detailed transform info
    QTreeWidgetItem* item = new QTreeWidgetItem(m_timelineTree);
    item->setText(0, boneItem->text());
    item->setText(1, QString::number(currentTime, 'f', 2));

    // Create detailed transform description
    QString transformDesc = QString("P(%.2f,%.2f,%.2f) R(%.1f,%.1f,%.1f) S(%.2f,%.2f,%.2f)")
        .arg(bone->position.x).arg(bone->position.y).arg(bone->position.z)
        .arg(glm::degrees(glm::eulerAngles(bone->rotation).x))
        .arg(glm::degrees(glm::eulerAngles(bone->rotation).y))
        .arg(glm::degrees(glm::eulerAngles(bone->rotation).z))
        .arg(bone->scale.x).arg(bone->scale.y).arg(bone->scale.z);

    item->setText(2, transformDesc);

    // Store keyframe data for easy access
    item->setData(0, Qt::UserRole, boneId);
    item->setData(1, Qt::UserRole, animationId);
    item->setData(2, Qt::UserRole, currentTime);

    m_timelineTree->addTopLevelItem(item);

    // Sort timeline items by time
    m_timelineTree->sortItems(1, Qt::AscendingOrder);
}

void VoxelBlockerDialog::onRemoveKeyframeClicked() {
    // Remove selected keyframe
    QTreeWidgetItem* item = m_timelineTree->currentItem();
    if (!item || !m_canvas) {
        return;
    }

    // Get keyframe data
    int boneId = item->data(0, Qt::UserRole).toInt();
    int animationId = item->data(1, Qt::UserRole).toInt();
    float time = item->data(2, Qt::UserRole).toFloat();

    // Remove from animation system
    m_canvas->RemoveKeyframe(animationId, boneId, time);

    // Remove from timeline tree
    delete item;
}

void VoxelBlockerDialog::onTimelineSelectionChanged() {
    // Update UI when timeline selection changes
    QTreeWidgetItem* item = m_timelineTree->currentItem();
    if (item) {
        // Parse time from item and update slider
        bool ok;
        float time = item->text(1).toFloat(&ok);
        if (ok) {
            m_timelineSlider->setValue(static_cast<int>(time * 1000));
        }
    }
}

void VoxelBlockerDialog::onKeyframeDoubleClicked(QTreeWidgetItem* item, int column) {
    // Allow editing keyframe values
    if (item && column == 1) {
        // Enable editing of time value
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        m_timelineTree->editItem(item, column);
    }
}

// Dock widget management slots
void VoxelBlockerDialog::onDockVisibilityChanged(bool visible) {
    // Update menu checkboxes when dock visibility changes
    Q_UNUSED(visible)
    // Implementation would update View menu items
}

void VoxelBlockerDialog::resetDockLayout() {
    // Reset dock widgets to default layout
    addDockWidget(Qt::LeftDockWidgetArea, m_toolsDock);
    addDockWidget(Qt::RightDockWidgetArea, m_animationDock);
    addDockWidget(Qt::BottomDockWidgetArea, m_timelineDock);

    // Reset sizes
    resizeDocks({m_toolsDock}, {350}, Qt::Horizontal);
    resizeDocks({m_animationDock}, {350}, Qt::Horizontal);
    resizeDocks({m_timelineDock}, {200}, Qt::Vertical);
}

// Context menu implementations
void VoxelBlockerDialog::showBoneContextMenu(const QPoint& pos) {
    QListWidgetItem* item = m_bonesList->itemAt(pos);
    if (!item) return;

    int boneId = item->data(Qt::UserRole).toInt();

    QMenu contextMenu(this);

    // Basic operations
    QAction* renameAction = contextMenu.addAction("Rename");
    QAction* deleteAction = contextMenu.addAction("Delete");

    contextMenu.addSeparator();

    // Hierarchy operations
    QAction* setParentAction = contextMenu.addAction("Set Parent...");
    QAction* removeParentAction = contextMenu.addAction("Remove Parent");

    // Check if bone has parent to enable/disable remove parent action
    if (m_canvas) {
        VoxelBone* bone = m_canvas->GetBone(boneId);
        removeParentAction->setEnabled(bone && bone->parentId >= 0);
    }

    contextMenu.addSeparator();

    // Animation operations
    QAction* setKeyframeAction = contextMenu.addAction("Set Keyframe");

    // Execute menu
    QAction* selectedAction = contextMenu.exec(m_bonesList->mapToGlobal(pos));

    if (selectedAction == renameAction) {
        onRenameBone();
    } else if (selectedAction == deleteAction) {
        onDeleteBoneClicked();
    } else if (selectedAction == setParentAction) {
        onSetBoneParent();
    } else if (selectedAction == removeParentAction) {
        if (m_canvas) {
            m_canvas->RemoveBoneParent(boneId);
        }
    } else if (selectedAction == setKeyframeAction) {
        onSetKeyframeClicked();
    }
}

void VoxelBlockerDialog::showAnimationContextMenu(const QPoint& pos) {
    QListWidgetItem* item = m_animationsList->itemAt(pos);
    if (!item) return;

    QMenu contextMenu(this);

    QAction* renameAction = contextMenu.addAction("Rename");
    QAction* deleteAction = contextMenu.addAction("Delete");
    contextMenu.addSeparator();
    QAction* duplicateAction = contextMenu.addAction("Duplicate");

    QAction* selectedAction = contextMenu.exec(m_animationsList->mapToGlobal(pos));

    if (selectedAction == renameAction) {
        onRenameAnimation();
    } else if (selectedAction == deleteAction) {
        onDeleteAnimationClicked();
    } else if (selectedAction == duplicateAction) {
        // TODO: Implement animation duplication
        QMessageBox::information(this, "Info", "Animation duplication not yet implemented");
    }
}

void VoxelBlockerDialog::onSetBoneParent() {
    QListWidgetItem* currentItem = m_bonesList->currentItem();
    if (!currentItem || !m_canvas) return;

    int boneId = currentItem->data(Qt::UserRole).toInt();

    // Create dialog to select parent bone
    QDialog dialog(this);
    dialog.setWindowTitle("Select Parent Bone");
    dialog.setModal(true);

    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    QLabel* label = new QLabel("Select parent bone:");
    layout->addWidget(label);

    QListWidget* parentList = new QListWidget();
    layout->addWidget(parentList);

    // Populate with available bones (excluding self and descendants)
    for (int i = 0; i < m_bonesList->count(); ++i) {
        QListWidgetItem* item = m_bonesList->item(i);
        int itemBoneId = item->data(Qt::UserRole).toInt();

        // Skip self and descendants to prevent circular dependencies
        if (itemBoneId != boneId && !m_canvas->IsBoneAncestor(boneId, itemBoneId)) {
            QListWidgetItem* parentItem = new QListWidgetItem(item->text());
            parentItem->setData(Qt::UserRole, itemBoneId);
            parentList->addItem(parentItem);
        }
    }

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* okButton = new QPushButton("OK");
    QPushButton* cancelButton = new QPushButton("Cancel");
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        QListWidgetItem* selectedParent = parentList->currentItem();
        if (selectedParent) {
            int parentId = selectedParent->data(Qt::UserRole).toInt();
            m_canvas->SetBoneParent(boneId, parentId);
        }
    }
}

void VoxelBlockerDialog::onRenameBone() {
    QListWidgetItem* currentItem = m_bonesList->currentItem();
    if (!currentItem) return;

    bool ok;
    QString newName = QInputDialog::getText(this, "Rename Bone", "Enter new name:",
                                          QLineEdit::Normal, currentItem->text(), &ok);

    if (ok && !newName.isEmpty()) {
        currentItem->setText(newName);

        // Update bone name in canvas
        if (m_canvas) {
            int boneId = currentItem->data(Qt::UserRole).toInt();
            VoxelBone* bone = m_canvas->GetBone(boneId);
            if (bone) {
                bone->name = newName;
                emit m_canvas->sceneModified();
            }
        }
    }
}

void VoxelBlockerDialog::onRenameAnimation() {
    QListWidgetItem* currentItem = m_animationsList->currentItem();
    if (!currentItem) return;

    bool ok;
    QString newName = QInputDialog::getText(this, "Rename Animation", "Enter new name:",
                                          QLineEdit::Normal, currentItem->text(), &ok);

    if (ok && !newName.isEmpty()) {
        currentItem->setText(newName);

        // Update animation name in canvas
        if (m_canvas) {
            int animId = currentItem->data(Qt::UserRole).toInt();
            // TODO: Add GetAnimation method to canvas and update name
            emit m_canvas->sceneModified();
        }
    }
}
