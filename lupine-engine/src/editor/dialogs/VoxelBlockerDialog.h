#pragma once

// Include glad before any Qt OpenGL headers to avoid conflicts
#include <glad/glad.h>

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QDialog>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QListWidgetItem>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QColorDialog>
#include <QtWidgets/QFrame>
#include <QtWidgets/QMenu>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QTreeWidgetItem>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QtGui/QActionGroup>
#include <QtGui/QColor>
#include <QtCore/QTimer>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include <vector>
#include "../rendering/GizmoRenderer.h"

/**
 * @brief Placement modes for voxels
 */
enum class VoxelPlacementMode {
    GridSnap,       // Snap to grid
    FaceSnap,       // Snap to existing voxel faces
    FreePlace       // Free placement
};

/**
 * @brief Advanced voxel editing tools
 */
enum class VoxelTool {
    Place,          // Standard placement tool
    Erase,          // Erase tool
    Paint,          // Paint existing voxels
    Select,         // Selection tool
    Brush,          // Brush tool for multiple voxels
    FloodFill,      // Flood fill tool
    Line,           // Line drawing tool
    Rectangle,      // Rectangle tool
    Sphere,         // Sphere tool
    Copy,           // Copy tool
    Paste           // Paste tool
};

/**
 * @brief Gizmo interaction modes
 */
enum class GizmoMode {
    None,
    Move,
    Rotate,
    Scale
};

/**
 * @brief Symmetry modes for voxel editing
 */
enum class SymmetryMode {
    None,           // No symmetry
    X,              // Mirror across X axis
    Y,              // Mirror across Y axis
    Z,              // Mirror across Z axis
    XY,             // Mirror across X and Y axes
    XZ,             // Mirror across X and Z axes
    YZ,             // Mirror across Y and Z axes
    XYZ             // Mirror across all axes
};

/**
 * @brief Represents a single voxel block
 */
struct Voxel {
    glm::vec3 position;         // Current world position
    glm::vec3 originalPosition; // Original position before bone transforms
    QColor color;               // Voxel color
    float size;                 // Voxel size
    bool selected;              // Whether this voxel is selected
    int boneId;                 // ID of bone this voxel is assigned to (-1 = none)

    Voxel(const glm::vec3& pos = glm::vec3(0.0f), const QColor& col = Qt::white, float sz = 1.0f)
        : position(pos), originalPosition(pos), color(col), size(sz), selected(false), boneId(-1) {}

    // Equality operator for std::vector comparison
    bool operator==(const Voxel& other) const {
        return glm::distance(position, other.position) < 0.001f &&
               color == other.color &&
               std::abs(size - other.size) < 0.001f &&
               selected == other.selected &&
               boneId == other.boneId;
    }

    bool operator!=(const Voxel& other) const {
        return !(*this == other);
    }
};

/**
 * @brief Represents a face of a voxel for export
 */
struct Face {
    glm::vec3 vertices[4];  // Four vertices of the quad face
    glm::vec3 normal;       // Face normal
    glm::vec3 center;       // Face center
    QColor color;           // Face color
    bool isExternal;        // Whether this face is external (visible)

    Face() : normal(0.0f), center(0.0f), color(Qt::white), isExternal(false) {
        for (int i = 0; i < 4; ++i) {
            vertices[i] = glm::vec3(0.0f);
        }
    }
};

/**
 * @brief Represents a bone for voxel rigging
 */
struct VoxelBone {
    int id;                     // Unique bone ID
    QString name;               // Bone name
    glm::vec3 position;         // Current bone position (joint location)
    glm::quat rotation;         // Current bone rotation (quaternion)
    glm::vec3 scale;            // Current bone scale

    // Rest pose (bind pose) transforms
    glm::vec3 restPosition;     // Rest position
    glm::quat restRotation;     // Rest rotation
    glm::vec3 restScale;        // Rest scale

    int parentId;               // Parent bone ID (-1 for root)
    std::vector<int> childIds;  // Child bone IDs
    QColor debugColor;          // Color for debug visualization
    bool visible;               // Whether bone is visible in editor

    // World transforms (computed from hierarchy)
    glm::vec3 worldPosition;    // World space position
    glm::quat worldRotation;    // World space rotation
    glm::vec3 worldScale;       // World space scale

    VoxelBone(int boneId = -1, const QString& boneName = "Bone")
        : id(boneId), name(boneName), position(0.0f), rotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)), scale(1.0f)
        , restPosition(0.0f), restRotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)), restScale(1.0f)
        , parentId(-1), debugColor(Qt::yellow), visible(true)
        , worldPosition(0.0f), worldRotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)), worldScale(1.0f) {}

    // Equality operator for std::vector comparison
    bool operator==(const VoxelBone& other) const {
        return id == other.id &&
               name == other.name &&
               glm::distance(position, other.position) < 0.001f &&
               glm::dot(rotation, other.rotation) > 0.999f && // Compare quaternions using dot product
               glm::distance(scale, other.scale) < 0.001f &&
               parentId == other.parentId &&
               childIds == other.childIds &&
               debugColor == other.debugColor &&
               visible == other.visible;
    }

    bool operator!=(const VoxelBone& other) const {
        return !(*this == other);
    }
};

/**
 * @brief Represents a keyframe for bone animation
 */
struct BoneKeyframe {
    float time;                 // Time in seconds
    glm::vec3 position;         // Position at this keyframe
    glm::quat rotation;         // Rotation at this keyframe (quaternion)
    glm::vec3 scale;            // Scale at this keyframe

    BoneKeyframe(float t = 0.0f, const glm::vec3& pos = glm::vec3(0.0f),
                 const glm::quat& rot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f), const glm::vec3& scl = glm::vec3(1.0f))
        : time(t), position(pos), rotation(rot), scale(scl) {}

    // Equality operator for std::vector comparison
    bool operator==(const BoneKeyframe& other) const {
        return std::abs(time - other.time) < 0.001f &&
               glm::distance(position, other.position) < 0.001f &&
               glm::dot(rotation, other.rotation) > 0.999f && // Compare quaternions using dot product
               glm::distance(scale, other.scale) < 0.001f;
    }

    bool operator!=(const BoneKeyframe& other) const {
        return !(*this == other);
    }
};

/**
 * @brief Represents an animation track for a single bone
 */
struct BoneAnimationTrack {
    int boneId;                             // Which bone this track animates
    std::vector<BoneKeyframe> keyframes;    // Keyframes for this bone

    BoneAnimationTrack(int id = -1) : boneId(id) {}

    // Equality operator for std::vector comparison
    bool operator==(const BoneAnimationTrack& other) const {
        return boneId == other.boneId && keyframes == other.keyframes;
    }

    bool operator!=(const BoneAnimationTrack& other) const {
        return !(*this == other);
    }
};

/**
 * @brief Represents a complete animation
 */
struct VoxelAnimation {
    QString name;                                   // Animation name
    float duration;                                 // Total duration in seconds
    bool looping;                                   // Whether animation loops
    std::vector<BoneAnimationTrack> tracks;         // Animation tracks per bone

    VoxelAnimation(const QString& animName = "Animation", float dur = 1.0f)
        : name(animName), duration(dur), looping(true) {}

    // Equality operator for std::vector comparison
    bool operator==(const VoxelAnimation& other) const {
        return name == other.name &&
               std::abs(duration - other.duration) < 0.001f &&
               looping == other.looping &&
               tracks == other.tracks;
    }

    bool operator!=(const VoxelAnimation& other) const {
        return !(*this == other);
    }
};

/**
 * @brief Clipboard data for copy/paste operations
 */
struct VoxelClipboard {
    std::vector<Voxel> voxels;
    std::vector<VoxelBone> bones;
    std::vector<VoxelAnimation> animations;
    glm::vec3 centerPoint;
    glm::vec3 boundingBoxMin;
    glm::vec3 boundingBoxMax;
    bool hasBones;
    bool hasAnimations;

    VoxelClipboard() : centerPoint(0.0f), boundingBoxMin(0.0f), boundingBoxMax(0.0f),
                      hasBones(false), hasAnimations(false) {}

    void Clear() {
        voxels.clear();
        bones.clear();
        animations.clear();
        hasBones = false;
        hasAnimations = false;
    }
};

/**
 * @brief Brush shapes for advanced tools
 */
enum class BrushShape {
    Sphere,         // Spherical brush
    Cube,           // Cubic brush
    Cylinder        // Cylindrical brush
};

/**
 * @brief Brush settings for advanced tools
 */
struct BrushSettings {
    float size;                 // Brush size/radius
    float strength;             // Brush strength (0.0 - 1.0)
    BrushShape shape;           // Brush shape
    bool spherical;             // Whether brush is spherical or cubic
    bool randomize;             // Add randomization to brush
    float randomStrength;       // Strength of randomization

    BrushSettings() : size(2.0f), strength(1.0f), shape(BrushShape::Sphere), spherical(true), randomize(false), randomStrength(0.1f) {}
};

/**
 * @brief Types of operations that can be undone/redone
 */
enum class UndoActionType {
    AddVoxel,
    RemoveVoxel,
    ModifyVoxel,
    AddBone,
    RemoveBone,
    ModifyBone,
    AddAnimation,
    RemoveAnimation,
    ModifyAnimation,
    BulkOperation
};

/**
 * @brief Represents a single undoable action
 */
struct UndoAction {
    UndoActionType type;
    QString description;

    // Voxel data
    std::vector<Voxel> voxelsBefore;
    std::vector<Voxel> voxelsAfter;

    // Bone data
    std::vector<VoxelBone> bonesBefore;
    std::vector<VoxelBone> bonesAfter;

    // Animation data
    std::vector<VoxelAnimation> animationsBefore;
    std::vector<VoxelAnimation> animationsAfter;

    // Specific item IDs for targeted operations
    std::vector<int> affectedVoxelIndices;
    std::vector<int> affectedBoneIds;
    std::vector<int> affectedAnimationIds;

    UndoAction(UndoActionType actionType, const QString& desc = "")
        : type(actionType), description(desc) {}
};

/**
 * @brief 3D viewport widget for voxel editing
 */
class VoxelCanvas : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit VoxelCanvas(QWidget* parent = nullptr);
    ~VoxelCanvas();

    // Voxel operations
    void AddVoxel(const glm::vec3& position, const QColor& color, float size);
    void RemoveVoxel(const glm::vec3& position);
    void ClearVoxels();

    // Selection operations
    void SelectVoxel(const glm::vec3& position);
    void ClearSelection();
    Voxel* GetSelectedVoxel() { return m_selectedVoxel; }
    std::vector<Voxel*> GetSelectedVoxels();
    void MoveSelectedVoxel(const glm::vec3& newPosition);
    void MoveSelectedVoxels(const glm::vec3& offset);
    void DeleteSelectedVoxel();
    void DeleteSelectedVoxels();
    void AddToSelection(const glm::vec3& position);
    void RemoveFromSelection(const glm::vec3& position);
    void SelectAll();
    void SelectInBox(const glm::vec3& min, const glm::vec3& max);

    // Multi-selection operations
    void InvertSelection();
    int GetSelectedVoxelCount() const;
    void SetSelectedVoxelsColor(const QColor& color);

    // Bone operations
    int CreateBone(const QString& name, const glm::vec3& position, int parentId = -1);
    void DeleteBone(int boneId);
    void SelectBone(int boneId);
    void ClearBoneSelection();
    VoxelBone* GetSelectedBone() { return m_selectedBone; }
    VoxelBone* GetBone(int boneId);
    const std::vector<VoxelBone>& GetBones() const { return m_bones; }
    void MoveBone(int boneId, const glm::vec3& newPosition);
    void RotateBone(int boneId, const glm::quat& rotation);
    void ScaleBone(int boneId, const glm::vec3& scale);

    // Bone hierarchy
    void SetBoneParent(int boneId, int parentId);
    void RemoveBoneParent(int boneId);
    bool IsBoneAncestor(int ancestorId, int descendantId) const;
    void UpdateBoneHierarchy(int boneId);
    std::vector<int> GetBoneChildren(int boneId) const;
    void AssignVoxelToBone(const glm::vec3& voxelPosition, int boneId);
    void UnassignVoxelFromBone(const glm::vec3& voxelPosition);
    void UpdateBoneTransforms();
    void SetBoneKeyframe(int boneId, float time);
    void DeleteBoneKeyframe(int boneId, float time);
    float GetCurrentAnimationTime() const { return m_currentAnimationTime; }

    // Animation operations
    int CreateAnimation(const QString& name, float duration = 1.0f);
    void DeleteAnimation(int animationId);
    void SelectAnimation(int animationId);
    VoxelAnimation* GetSelectedAnimation() { return m_selectedAnimation; }
    const std::vector<VoxelAnimation>& GetAnimations() const { return m_animations; }
    void AddKeyframe(int animationId, int boneId, float time, const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale);
    void RemoveKeyframe(int animationId, int boneId, float time);
    void PlayAnimation(int animationId);
    void StopAnimation();
    void SetAnimationTime(float time);
    void SetAnimationSpeed(float speed) { m_animationSpeed = speed; }
    float GetAnimationSpeed() const { return m_animationSpeed; }
    float GetAnimationDuration(int animationId) const;
    void SetAnimationLooping(int animationId, bool looping);
    bool IsAnimationLooping(int animationId) const;
    void RenameAnimation(int animationId, const QString& newName);
    void SetAnimationDuration(int animationId, float duration);

    // Keyframe management
    std::vector<float> GetKeyframeTimes(int animationId, int boneId) const;
    BoneKeyframe* GetKeyframe(int animationId, int boneId, float time);
    void UpdateKeyframe(int animationId, int boneId, float time, const BoneKeyframe& keyframe);
    void MoveKeyframe(int animationId, int boneId, float oldTime, float newTime);
    void CopyKeyframe(int animationId, int boneId, float time);
    void PasteKeyframe(int animationId, int boneId, float time);

    // Animation export
    bool ExportAnimationToJSON(int animationId, const QString& filepath);
    bool ImportAnimationFromJSON(const QString& filepath);
    bool ExportAllAnimationsToJSON(const QString& filepath);

    // Rigging mode
    void SetRiggingMode(bool enabled) { m_riggingMode = enabled; update(); }
    bool IsRiggingMode() const { return m_riggingMode; }
    void SetShowBones(bool show) { m_showBones = show; update(); }
    bool GetShowBones() const { return m_showBones; }

    // Advanced tools
    void SetCurrentTool(VoxelTool tool) { m_currentTool = tool; }
    VoxelTool GetCurrentTool() const { return m_currentTool; }
    void SetSymmetryMode(SymmetryMode mode) { m_symmetryMode = mode; }
    SymmetryMode GetSymmetryMode() const { return m_symmetryMode; }
    void SetSymmetryCenter(const glm::vec3& center) { m_symmetryCenter = center; }
    glm::vec3 GetSymmetryCenter() const { return m_symmetryCenter; }

    // Brush operations
    void SetBrushSettings(const BrushSettings& settings) { m_brushSettings = settings; }
    BrushSettings GetBrushSettings() const { return m_brushSettings; }
    void BrushPaint(const glm::vec3& center, const QColor& color);
    void BrushErase(const glm::vec3& center);

    // Advanced placement tools
    void DrawLine(const glm::vec3& start, const glm::vec3& end, const QColor& color, float size);
    void DrawRectangle(const glm::vec3& corner1, const glm::vec3& corner2, const QColor& color, float size, bool filled = false);
    void DrawSphere(const glm::vec3& center, float radius, const QColor& color, float voxelSize, bool filled = true);
    void FloodFill(const glm::vec3& startPos, const QColor& newColor);

    // Copy/paste operations
    void CopySelection(const glm::vec3& min, const glm::vec3& max, bool includeBones = true, bool includeAnimations = false);
    void CopyVoxels(const std::vector<glm::vec3>& positions, bool includeBones = true, bool includeAnimations = false);
    void CopySelectedVoxels(bool includeBones = true, bool includeAnimations = false);
    void CopyAll(bool includeBones = true, bool includeAnimations = true);
    void Paste(const glm::vec3& position, bool pasteBones = true, bool pasteAnimations = false);
    void PasteWithOffset(const glm::vec3& offset, bool pasteBones = true, bool pasteAnimations = false);
    bool HasClipboardData() const { return !m_clipboard.voxels.empty(); }
    bool HasClipboardBones() const { return m_clipboard.hasBones; }
    bool HasClipboardAnimations() const { return m_clipboard.hasAnimations; }
    void ClearClipboard() { m_clipboard.Clear(); }
    VoxelClipboard GetClipboardData() const { return m_clipboard; }

    // Utility functions
    std::vector<glm::vec3> GetVoxelsInRadius(const glm::vec3& center, float radius, bool spherical = true);
    std::vector<glm::vec3> GetSymmetryPositions(const glm::vec3& position);
    void ApplySymmetry(const glm::vec3& position, const QColor& color, float size);

    // Undo/Redo system
    void Undo();
    void Redo();
    bool CanUndo() const { return m_undoIndex > 0; }
    bool CanRedo() const { return m_undoIndex < m_undoStack.size(); }
    void ClearUndoStack();
    void BeginUndoGroup(const QString& description);
    void EndUndoGroup();
    QString GetUndoDescription() const;
    QString GetRedoDescription() const;

    // Settings
    void SetVoxelSize(float size) { m_voxelSize = size; }
    float GetVoxelSize() const { return m_voxelSize; }
    
    void SetVoxelColor(const QColor& color) { m_voxelColor = color; }
    QColor GetVoxelColor() const { return m_voxelColor; }
    
    void SetPlacementMode(VoxelPlacementMode mode) { m_placementMode = mode; }
    VoxelPlacementMode GetPlacementMode() const { return m_placementMode; }
    
    void SetGridSize(float size) { m_gridSize = size; }
    float GetGridSize() const { return m_gridSize; }

    void SetGridBaseY(float baseY) { m_gridBaseY = baseY; update(); }
    float GetGridBaseY() const { return m_gridBaseY; }

    void SetShowGrid(bool show) { m_showGrid = show; update(); }
    bool GetShowGrid() const { return m_showGrid; }

    int GetCurrentFace() const { return m_currentFace; }

    const std::vector<Voxel>& GetVoxels() const { return m_voxels; }

    // Camera controls
    void ResetCamera();
    void FocusOnVoxels();

    // Gizmo operations
    void SetGizmoMode(GizmoMode mode) { m_gizmoMode = mode; update(); }
    GizmoMode GetGizmoMode() const { return m_gizmoMode; }
    GizmoAxis GetHoveredGizmoAxis(const QPoint& screenPos);
    void StartGizmoInteraction(GizmoAxis axis, const QPoint& screenPos);
    void UpdateGizmoInteraction(const QPoint& screenPos);
    void EndGizmoInteraction();

    // Context menu
    void showContextMenu(const QPoint& pos);

    // File operations
    void NewScene();
    bool LoadFromFile(const QString& filepath);
    bool SaveToFile(const QString& filepath);
    bool ExportToOBJ(const QString& filepath);
    bool ExportToOBJ(const QString& filepath, bool mergeFaces);
    bool ExportToOBJ(const QString& filepath, bool mergeFaces, bool useTextureAtlas);
    bool ExportToFBX(const QString& filepath, bool useTextureAtlas = false);
    bool generateTextureAtlas(const QString& basePath, std::map<uint32_t, glm::vec2>& colorToUV);

signals:
    void voxelAdded(const glm::vec3& position);
    void voxelRemoved(const glm::vec3& position);
    void voxelSelected(Voxel* voxel);
    void voxelDeselected();
    void boneCreated(int boneId);
    void boneDeleted(int boneId);
    void boneSelected(VoxelBone* bone);
    void boneDeselected();
    void animationCreated(int animationId);
    void animationDeleted(int animationId);
    void animationSelected(VoxelAnimation* animation);
    void sceneModified();
    void faceChanged(int face);
    void gridBaseYChanged(float baseY);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void updateAnimation();

private:
    void setupShaders();
    void setupBuffers();
    void updateBuffers();
    
    void drawGrid();
    void drawVoxels();
    void drawPreviewVoxel();
    void drawBones();
    void drawGizmos();
    void drawGizmoAxis(const glm::vec3& origin, const glm::vec3& direction, const glm::vec3& color);
    void drawMoveGizmo(const glm::vec3& position, float scale);
    void drawRotateGizmo(const glm::vec3& position, float scale);
    void drawScaleGizmo(const glm::vec3& position, float scale);
    void drawCube(float size);

    // Symmetry helpers
    std::vector<glm::vec3> getSymmetryPositions(const glm::vec3& position);

    glm::vec3 screenToWorld(const QPoint& screenPos);
    glm::vec3 snapToGrid(const glm::vec3& position);
    glm::vec3 snapToFace(const glm::vec3& position);
    Voxel* getVoxelAt(const glm::vec3& position);
    glm::vec3 getSnapPosition(const glm::vec3& worldPos);
    
    void updateCamera();
    void updatePreview(const QPoint& mousePos);

    // Texture atlas generation
    std::vector<glm::vec2> getQuadUVs(uint32_t colorRGB, int atlasSize, int textureSize, int totalSize);

    // Face merging
    std::vector<Face> mergeFacesAlgorithm(const std::vector<Face>& faces);
    bool canMergeFaces(const Face& face1, const Face& face2);
    Face mergeTwoFaces(const Face& face1, const Face& face2);

    // Undo system helpers
    void RecordUndoAction(UndoActionType type, const QString& description = "");

    // Rendering
    glm::mat4 m_projectionMatrix;
    glm::mat4 m_viewMatrix;
    glm::mat4 m_modelMatrix;

    // OpenGL objects
    unsigned int m_shaderProgram;
    unsigned int m_cubeVAO, m_cubeVBO, m_cubeEBO;
    unsigned int m_gridVAO, m_gridVBO;
    unsigned int m_colorVBO;

    // Camera
    glm::vec3 m_cameraPosition;
    glm::vec3 m_cameraTarget;
    glm::vec3 m_cameraUp;
    float m_cameraDistance;
    float m_cameraYaw;
    float m_cameraPitch;
    
    // Mouse interaction
    bool m_mousePressed;
    QPoint m_lastMousePos;
    Qt::MouseButton m_pressedButton;
    int m_currentFace; // For face cycling with middle mouse
    bool m_isPanning;
    bool m_isOrbiting;
    
    // Voxel data
    std::vector<Voxel> m_voxels;
    Voxel* m_selectedVoxel;  // Currently selected voxel for editing

    // Bone and animation data
    std::vector<VoxelBone> m_bones;
    std::vector<VoxelAnimation> m_animations;
    VoxelBone* m_selectedBone;
    VoxelAnimation* m_selectedAnimation;
    int m_nextBoneId;
    int m_nextAnimationId;

    // Rigging and animation state
    bool m_riggingMode;
    bool m_showBones;
    bool m_isPlaying;
    float m_currentAnimationTime;
    int m_playingAnimationId;
    float m_animationSpeed;
    BoneKeyframe m_copiedKeyframe;
    bool m_hasKeyframeCopy;

    // Advanced tools state
    VoxelTool m_currentTool;
    SymmetryMode m_symmetryMode;
    glm::vec3 m_symmetryCenter;
    BrushSettings m_brushSettings;
    VoxelClipboard m_clipboard;

    // Tool interaction state
    bool m_isDragging;
    glm::vec3 m_dragStartPos;
    glm::vec3 m_dragCurrentPos;

    // Gizmo interaction state
    GizmoMode m_gizmoMode;
    GizmoAxis m_hoveredGizmoAxis;
    GizmoAxis m_activeGizmoAxis;
    bool m_gizmoInteracting;
    glm::vec3 m_gizmoStartPos;
    glm::vec3 m_gizmoCurrentPos;
    std::vector<glm::vec3> m_selectionStartPositions;

    // Undo/Redo system
    std::vector<UndoAction> m_undoStack;
    size_t m_undoIndex;
    size_t m_maxUndoSteps;
    bool m_recordingUndoGroup;
    UndoAction* m_currentUndoGroup;

    // Settings
    float m_voxelSize;
    QColor m_voxelColor;
    VoxelPlacementMode m_placementMode;
    float m_gridSize;
    float m_gridBaseY;
    bool m_showGrid;

    // Preview
    bool m_showPreview;
    glm::vec3 m_previewPosition;

    // Animation timer
    QTimer* m_animationTimer;
};

/**
 * @brief Dialog for creating and editing voxel objects
 *
 * This dialog provides a complete 3D voxel editing environment with:
 * - 3D viewport with camera controls
 * - Voxel placement with grid/face/free snapping
 * - Color and size controls
 * - Export to OBJ/FBX with vertex colors
 * - Save/load .voxels format
 * - Dockable panels for modular layout
 */
class VoxelBlockerDialog : public QMainWindow {
    Q_OBJECT

public:
    explicit VoxelBlockerDialog(QWidget* parent = nullptr);
    ~VoxelBlockerDialog();

private slots:
    // File operations
    void onNewScene();
    void onOpenFile();
    void onSaveFile();
    void onSaveAs();
    void onExportOBJ();
    void onExportFBX();

    // Edit operations
    void onUndo();
    void onRedo();
    void onCopy();
    void onPaste();
    void onCut();

    // Tool selection
    void onToolChanged(int toolId);
    void onPlaceToolSelected();
    void onEraseToolSelected();
    void onSelectToolSelected();
    void onFloodFillToolSelected();
    void onLineToolSelected();
    void onRectangleToolSelected();
    void onSphereToolSelected();

    // Gizmo mode selection
    void onGizmoModeChanged(GizmoMode mode);

    // Tool settings
    void onVoxelSizeChanged();
    void onVoxelColorChanged();
    void onPlacementModeChanged();
    void onGridSizeChanged();
    void onShowGridChanged();

    // Grid controls
    void onGridBaseYChanged();
    void onGridUpClicked();
    void onGridDownClicked();

    // Face cycling
    void onFaceChanged(int face);

    // Animation system
    void onRiggingModeChanged(bool enabled);
    void onShowBonesChanged(bool show);
    void onCreateBoneClicked();
    void onDeleteBoneClicked();
    void onAssignBoneClicked();
    void onBoneSelectionChanged();

    // Bone transform controls
    void updateBoneTransformUI(int boneId);
    void onBoneTransformChanged();
    void onSetKeyframeClicked();
    void onDeleteKeyframeClicked();

    // Animation controls
    void onCreateAnimationClicked();
    void onDeleteAnimationClicked();
    void onPlayAnimationClicked();
    void onStopAnimationClicked();
    void onAnimationSelectionChanged();
    void onAnimationTimeChanged();
    void onAnimationSpeedChanged();
    void onAnimationDurationChanged();

    // Timeline controls
    void onAddKeyframeClicked();
    void onRemoveKeyframeClicked();
    void onTimelineSelectionChanged();
    void onKeyframeDoubleClicked(QTreeWidgetItem* item, int column);

    // Dock widget management
    void onDockVisibilityChanged(bool visible);
    void resetDockLayout();

    // Advanced tools
    void onSymmetryModeChanged();
    void onSymmetryCenterChanged();

    // Camera controls
    void onResetCamera();
    void onFocusOnVoxels();

    // Context menus
    void showBoneContextMenu(const QPoint& pos);
    void showAnimationContextMenu(const QPoint& pos);
    void onSetBoneParent();
    void onRenameBone();
    void onRenameAnimation();

private:
    void setVoxelColor(const QColor& color);

    // Scene events
    void onVoxelAdded(const glm::vec3& position);
    void onVoxelRemoved(const glm::vec3& position);
    void onSceneModified();

private:
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupMainPanels();
    void setupDockWidgets();
    void setupToolPanel();
    void setupAnimationPanel();
    void setupTimelinePanel();
    void setupViewportPanel();

    void updateWindowTitle();
    void updateVoxelCount();
    void updateUndoRedoActions();

    // Utility functions
    bool hasUnsavedChanges() const;
    bool promptSaveChanges();
    void setModified(bool modified);
    void refreshTimeline();

    // UI Components
    QWidget* m_centralWidget;
    QMenuBar* m_menuBar;
    QToolBar* m_toolBar;

    // Menu actions
    QAction* m_undoAction;
    QAction* m_redoAction;

    // 3D Viewport
    VoxelCanvas* m_canvas;

    // Dock widgets
    QDockWidget* m_toolsDock;
    QDockWidget* m_animationDock;
    QDockWidget* m_timelineDock;

    // Tool panel
    QWidget* m_toolPanel;
    QScrollArea* m_toolScrollArea;

    // Tool selection
    QGroupBox* m_toolsGroup;
    QPushButton* m_placeToolButton;
    QPushButton* m_eraseToolButton;
    QPushButton* m_selectToolButton;
    QPushButton* m_floodFillToolButton;
    QPushButton* m_lineToolButton;
    QPushButton* m_rectangleToolButton;
    QPushButton* m_sphereToolButton;
    QButtonGroup* m_toolButtonGroup;

    // Gizmo mode selection
    QGroupBox* m_gizmoGroup;
    QPushButton* m_moveGizmoButton;
    QPushButton* m_rotateGizmoButton;
    QPushButton* m_scaleGizmoButton;
    QButtonGroup* m_gizmoButtonGroup;

    // Voxel settings
    QSlider* m_voxelSizeSlider;
    QDoubleSpinBox* m_voxelSizeSpinBox;
    QPushButton* m_voxelColorButton;
    QComboBox* m_placementModeCombo;
    QSlider* m_gridSizeSlider;
    QDoubleSpinBox* m_gridSizeSpinBox;
    QCheckBox* m_showGridCheck;

    // Grid controls
    QGroupBox* m_gridGroup;
    QLabel* m_gridBaseYLabel;
    QDoubleSpinBox* m_gridBaseYSpinBox;
    QPushButton* m_gridUpButton;
    QPushButton* m_gridDownButton;

    // Face cycling display
    QGroupBox* m_faceGroup;
    QLabel* m_currentFaceLabel;
    QLabel* m_faceDisplayLabel;

    // Animation system UI
    QGroupBox* m_animationGroup;
    QCheckBox* m_riggingModeCheck;
    QCheckBox* m_showBonesCheck;
    QPushButton* m_createBoneButton;
    QPushButton* m_deleteBoneButton;
    QPushButton* m_assignBoneButton;
    QListWidget* m_bonesList;

    // Bone transform controls
    QGroupBox* m_boneTransformGroup;
    QDoubleSpinBox* m_bonePositionXSpinBox;
    QDoubleSpinBox* m_bonePositionYSpinBox;
    QDoubleSpinBox* m_bonePositionZSpinBox;
    QDoubleSpinBox* m_boneRotationXSpinBox;
    QDoubleSpinBox* m_boneRotationYSpinBox;
    QDoubleSpinBox* m_boneRotationZSpinBox;
    QDoubleSpinBox* m_boneScaleXSpinBox;
    QDoubleSpinBox* m_boneScaleYSpinBox;
    QDoubleSpinBox* m_boneScaleZSpinBox;
    QPushButton* m_setKeyframeButton;
    QPushButton* m_deleteKeyframeButton;

    // Animation panel
    QWidget* m_animationPanel;
    QScrollArea* m_animationScrollArea;

    // Animation controls
    QGroupBox* m_animationControlsGroup;
    QListWidget* m_animationsList;
    QPushButton* m_createAnimationButton;
    QPushButton* m_deleteAnimationButton;
    QPushButton* m_playAnimationButton;
    QPushButton* m_stopAnimationButton;
    QSlider* m_animationTimeSlider;
    QDoubleSpinBox* m_animationTimeSpinBox;
    QDoubleSpinBox* m_animationSpeedSpinBox;
    QDoubleSpinBox* m_animationDurationSpinBox;
    QLabel* m_animationStatusLabel;

    // Timeline panel
    QWidget* m_timelinePanel;
    QTreeWidget* m_timelineTree;
    QPushButton* m_addKeyframeButton;
    QPushButton* m_removeKeyframeButton;
    QSlider* m_timelineSlider;
    QLabel* m_timelineLabel;

    // Advanced tools UI
    QGroupBox* m_advancedToolsGroup;
    QComboBox* m_symmetryModeCombo;
    QDoubleSpinBox* m_symmetryCenterXSpinBox;
    QDoubleSpinBox* m_symmetryCenterYSpinBox;
    QDoubleSpinBox* m_symmetryCenterZSpinBox;


    
    // Camera controls
    QPushButton* m_resetCameraButton;
    QPushButton* m_focusButton;
    
    // Info display
    QLabel* m_voxelCountLabel;
    QLabel* m_positionLabel;

    // State
    QString m_currentFilePath;
    bool m_modified;
    int m_voxelCount;
};
