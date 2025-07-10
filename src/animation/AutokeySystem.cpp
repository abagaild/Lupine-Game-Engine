#include "lupine/animation/AutokeySystem.h"
#include "lupine/core/Scene.h"
#include "lupine/resources/AnimationResource.h"
#include <algorithm>
#include <sstream>

namespace Lupine {

// AutokeySystem implementation
AutokeySystem::AutokeySystem()
    : m_autokeyMode(AutokeyMode::Disabled)
    , m_currentTime(0.0f)
    , m_autokeyEnabled(false)
    , m_isMonitoring(false)
    , m_batchMode(false)
    , m_scene(nullptr)
    , m_animationResource(nullptr)
{
}

AutokeySystem::~AutokeySystem() {
    StopMonitoring();
}

void AutokeySystem::SetScene(Scene* scene) {
    if (m_scene != scene) {
        StopMonitoring();
        m_scene = scene;
        m_initialState = SceneSnapshot();
        m_lastKnownState = SceneSnapshot();
    }
}

void AutokeySystem::StartMonitoring() {
    if (!m_scene || m_isMonitoring) return;
    
    m_isMonitoring = true;
    CaptureInitialState();
    UpdateLastKnownState();
}

void AutokeySystem::StopMonitoring() {
    m_isMonitoring = false;
}

void AutokeySystem::RecordKeyframe(Node* node, const std::string& propertyName) {
    if (!node || !m_animationResource || m_currentClip.empty()) return;
    
    if (!ShouldRecordProperty(node, propertyName)) return;
    
    std::string nodePath = GetNodePath(node);
    EnhancedAnimationValue value = m_reflectionSystem.GetPropertyValue(node, propertyName);
    
    if (value.IsValid()) {
        CreateKeyframe(nodePath, propertyName, value, m_currentTime);
        
        if (m_keyframeRecordedCallback) {
            m_keyframeRecordedCallback(nodePath, propertyName, m_currentTime);
        }
    }
}

void AutokeySystem::RecordKeyframes(Node* node, const std::vector<std::string>& propertyNames) {
    if (!node) return;
    
    BeginBatch();
    for (const auto& propertyName : propertyNames) {
        RecordKeyframe(node, propertyName);
    }
    EndBatch();
}

void AutokeySystem::RecordAllKeyframes(Node* node) {
    if (!node) return;
    
    auto properties = m_reflectionSystem.DiscoverProperties(node);
    std::vector<std::string> propertyNames;
    
    for (const auto& prop : properties) {
        if (m_propertyFilter.ShouldIncludeProperty(prop) && 
            m_reflectionSystem.IsPropertyAnimatable(prop)) {
            propertyNames.push_back(prop.name);
        }
    }
    
    RecordKeyframes(node, propertyNames);
}

void AutokeySystem::BeginBatch() {
    m_batchMode = true;
    m_batchedChanges.clear();
}

void AutokeySystem::EndBatch() {
    if (!m_batchMode) return;
    
    m_batchMode = false;
    
    // Process all batched changes
    for (const auto& change : m_batchedChanges) {
        ProcessPropertyChange(change);
    }
    
    m_batchedChanges.clear();
}

void AutokeySystem::OnPropertyChanged(Node* node, const std::string& propertyName, 
                                    const EnhancedAnimationValue& oldValue, const EnhancedAnimationValue& newValue) {
    if (!m_autokeyEnabled || !m_isMonitoring || !node) return;
    
    std::string nodePath = GetNodePath(node);
    PropertyChangeEvent change(nodePath, propertyName, oldValue, newValue, m_currentTime);
    
    if (m_batchMode) {
        m_batchedChanges.push_back(change);
    } else {
        ProcessPropertyChange(change);
    }
}

void AutokeySystem::CaptureInitialState() {
    if (!m_scene) return;
    
    m_initialState = m_stateManager.CaptureSceneState(m_scene, m_propertyFilter);
}

void AutokeySystem::RestoreInitialState() {
    if (!m_scene || m_initialState.nodeSnapshots.empty()) return;
    
    m_stateManager.RestoreSceneState(m_scene, m_initialState);
}

std::string AutokeySystem::GetNodePath(Node* node) const {
    if (!node) return "";
    
    std::vector<std::string> pathComponents;
    Node* current = node;
    
    while (current && current->GetParent()) {
        pathComponents.push_back(current->GetName());
        current = current->GetParent();
    }
    
    if (current) {
        pathComponents.push_back(current->GetName()); // Root node
    }
    
    // Reverse to get path from root to node
    std::reverse(pathComponents.begin(), pathComponents.end());
    
    std::ostringstream oss;
    for (size_t i = 0; i < pathComponents.size(); ++i) {
        if (i > 0) oss << "/";
        oss << pathComponents[i];
    }
    
    return oss.str();
}

Node* AutokeySystem::FindNodeByPath(const std::string& path) const {
    if (!m_scene || path.empty()) return nullptr;
    
    // Split path by '/'
    std::vector<std::string> pathComponents;
    std::istringstream iss(path);
    std::string component;
    
    while (std::getline(iss, component, '/')) {
        if (!component.empty()) {
            pathComponents.push_back(component);
        }
    }
    
    if (pathComponents.empty()) return nullptr;
    
    // Start from root and traverse
    Node* current = m_scene->GetRootNode();
    if (!current || current->GetName() != pathComponents[0]) {
        return nullptr;
    }

    for (size_t i = 1; i < pathComponents.size(); ++i) {
        Node* found = nullptr;
        for (const auto& child : current->GetChildren()) {
            if (child->GetName() == pathComponents[i]) {
                found = child.get();
                break;
            }
        }

        if (!found) return nullptr;
        current = found;
    }
    
    return current;
}

bool AutokeySystem::ShouldRecordProperty(Node* node, const std::string& propertyName) const {
    if (!node) return false;
    
    // Check autokey mode
    switch (m_autokeyMode) {
        case AutokeyMode::Disabled:
            return false;
        case AutokeyMode::AllProperties:
            break; // Continue with filter checks
        case AutokeyMode::SelectedProperties:
            // In a full implementation, this would check if the property is selected
            break;
        case AutokeyMode::ChangedProperties:
            // This would require tracking which properties actually changed
            break;
        case AutokeyMode::TransformOnly:
            if (propertyName != "position" && propertyName != "rotation" && propertyName != "scale") {
                return false;
            }
            break;
    }
    
    // Check if property is animatable
    auto properties = m_reflectionSystem.DiscoverProperties(node);
    auto it = std::find_if(properties.begin(), properties.end(),
                          [&propertyName](const PropertyDescriptor& desc) {
                              return desc.name == propertyName;
                          });
    
    if (it == properties.end() || !m_reflectionSystem.IsPropertyAnimatable(*it)) {
        return false;
    }
    
    // Check property filter
    return m_propertyFilter.ShouldIncludeProperty(*it);
}

void AutokeySystem::ProcessPropertyChange(const PropertyChangeEvent& change) {
    if (!ShouldRecordProperty(FindNodeByPath(change.nodePath), change.propertyName)) {
        return;
    }
    
    CreateKeyframe(change.nodePath, change.propertyName, change.newValue, change.timestamp);
    
    if (m_keyframeRecordedCallback) {
        m_keyframeRecordedCallback(change.nodePath, change.propertyName, change.timestamp);
    }
}

void AutokeySystem::CreateKeyframe(const std::string& nodePath, const std::string& propertyName, 
                                 const EnhancedAnimationValue& value, float time) {
    if (!m_animationResource || m_currentClip.empty()) return;
    
    AnimationTrack* track = FindOrCreateTrack(nodePath, propertyName);
    if (!track) return;
    
    // Convert enhanced value to animation value
    AnimationValue animValue = ConvertToAnimationValue(value);
    
    // Check if keyframe already exists at this time
    auto it = std::find_if(track->keyframes.begin(), track->keyframes.end(),
                          [time](const AnimationKeyframe& kf) {
                              return std::abs(kf.time - time) < 0.001f;
                          });
    
    if (it != track->keyframes.end()) {
        // Update existing keyframe
        it->value = animValue;
    } else {
        // Create new keyframe
        AnimationKeyframe keyframe(time, animValue, InterpolationType::Linear);
        
        // Insert in sorted order
        auto insertPos = std::upper_bound(track->keyframes.begin(), track->keyframes.end(), keyframe,
                                         [](const AnimationKeyframe& a, const AnimationKeyframe& b) {
                                             return a.time < b.time;
                                         });
        track->keyframes.insert(insertPos, keyframe);
    }
}

AnimationTrack* AutokeySystem::FindOrCreateTrack(const std::string& nodePath, const std::string& propertyName) {
    if (!m_animationResource || m_currentClip.empty()) return nullptr;
    
    AnimationClip* clip = m_animationResource->GetClip(m_currentClip);
    if (!clip) return nullptr;
    
    // Look for existing track
    for (auto& track : clip->tracks) {
        if (track.node_path == nodePath && track.property_name == propertyName) {
            return &track;
        }
    }
    
    // Create new track
    Node* node = FindNodeByPath(nodePath);
    if (!node) return nullptr;
    
    ExportVariableType exportType = m_reflectionSystem.GetPropertyType(node, propertyName);
    AnimationPropertyType animType = ConvertToAnimationPropertyType(exportType);
    
    AnimationTrack newTrack(nodePath, propertyName, animType);
    clip->tracks.push_back(newTrack);
    
    return &clip->tracks.back();
}

AnimationPropertyType AutokeySystem::ConvertToAnimationPropertyType(ExportVariableType exportType) const {
    switch (exportType) {
        case ExportVariableType::Bool: return AnimationPropertyType::Bool;
        case ExportVariableType::Int: return AnimationPropertyType::Int;
        case ExportVariableType::Float: return AnimationPropertyType::Float;
        case ExportVariableType::Vec2: return AnimationPropertyType::Vec2;
        case ExportVariableType::Vec3: return AnimationPropertyType::Vec3;
        case ExportVariableType::Vec4: return AnimationPropertyType::Vec4;
        default: return AnimationPropertyType::Float;
    }
}

AnimationValue AutokeySystem::ConvertToAnimationValue(const EnhancedAnimationValue& enhancedValue) const {
    switch (enhancedValue.type) {
        case ExportVariableType::Bool:
            return AnimationValue(enhancedValue.GetValue<bool>());
        case ExportVariableType::Int:
            return AnimationValue(enhancedValue.GetValue<int>());
        case ExportVariableType::Float:
            return AnimationValue(enhancedValue.GetValue<float>());
        case ExportVariableType::Vec2:
            return AnimationValue(enhancedValue.GetValue<glm::vec2>());
        case ExportVariableType::Vec3:
            return AnimationValue(enhancedValue.GetValue<glm::vec3>());
        case ExportVariableType::Vec4:
            return AnimationValue(enhancedValue.GetValue<glm::vec4>());
        default:
            return AnimationValue(0.0f);
    }
}

void AutokeySystem::UpdateLastKnownState() {
    if (!m_scene) return;
    
    m_lastKnownState = m_stateManager.CaptureSceneState(m_scene, m_propertyFilter);
}

void AutokeySystem::DetectChanges() {
    if (!m_scene || !m_isMonitoring) return;
    
    SceneSnapshot currentState = m_stateManager.CaptureSceneState(m_scene, m_propertyFilter);
    auto changes = m_stateManager.CompareSceneStates(m_lastKnownState, currentState);
    
    for (const auto& change : changes) {
        OnPropertyChanged(FindNodeByPath(change.nodePath), change.propertyName, 
                         change.oldValue, change.newValue);
    }
    
    m_lastKnownState = currentState;
}

} // namespace Lupine
