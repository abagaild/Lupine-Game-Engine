#pragma once

#include "lupine/animation/PropertySystem.h"
#include "lupine/resources/AnimationResource.h"
#include <functional>
#include <memory>
#include <queue>
#include <unordered_set>
#include <set>

namespace Lupine {

class Scene;
class TweenAnimationResource;

/**
 * @brief Autokey recording system for automatic keyframe creation
 */
class AutokeySystem {
public:
    AutokeySystem();
    ~AutokeySystem();
    
    // Configuration
    void SetAutokeyMode(AutokeyMode mode) { m_autokeyMode = mode; }
    AutokeyMode GetAutokeyMode() const { return m_autokeyMode; }
    
    void SetPropertyFilter(const PropertyFilter& filter) { m_propertyFilter = filter; }
    const PropertyFilter& GetPropertyFilter() const { return m_propertyFilter; }
    
    void SetCurrentTime(float time) { m_currentTime = time; }
    float GetCurrentTime() const { return m_currentTime; }
    
    void SetAnimationResource(TweenAnimationResource* resource) { m_animationResource = resource; }
    void SetCurrentClip(const std::string& clipName) { m_currentClip = clipName; }
    
    // Scene monitoring
    void SetScene(Scene* scene);
    Scene* GetScene() const { return m_scene; }
    
    // Autokey control
    void EnableAutokey(bool enabled = true) { m_autokeyEnabled = enabled; }
    void DisableAutokey() { m_autokeyEnabled = false; }
    bool IsAutokeyEnabled() const { return m_autokeyEnabled; }
    
    // Property monitoring
    void StartMonitoring();
    void StopMonitoring();
    bool IsMonitoring() const { return m_isMonitoring; }
    
    // Manual keyframe recording
    void RecordKeyframe(Node* node, const std::string& propertyName);
    void RecordKeyframes(Node* node, const std::vector<std::string>& propertyNames);
    void RecordAllKeyframes(Node* node);
    
    // Batch operations
    void BeginBatch();
    void EndBatch();
    bool IsInBatch() const { return m_batchMode; }
    
    // Property change detection
    void OnPropertyChanged(Node* node, const std::string& propertyName, 
                          const EnhancedAnimationValue& oldValue, const EnhancedAnimationValue& newValue);
    
    // Callbacks
    using KeyframeRecordedCallback = std::function<void(const std::string& nodePath, const std::string& propertyName, float time)>;
    void SetKeyframeRecordedCallback(const KeyframeRecordedCallback& callback) { m_keyframeRecordedCallback = callback; }
    
    // State management
    void CaptureInitialState();
    void RestoreInitialState();
    bool HasInitialState() const { return !m_initialState.nodeSnapshots.empty(); }
    
    // Utility methods
    std::string GetNodePath(Node* node) const;
    Node* FindNodeByPath(const std::string& path) const;
    
private:
    // Internal state
    AutokeyMode m_autokeyMode;
    PropertyFilter m_propertyFilter;
    float m_currentTime;
    bool m_autokeyEnabled;
    bool m_isMonitoring;
    bool m_batchMode;
    
    // Scene and animation data
    Scene* m_scene;
    TweenAnimationResource* m_animationResource;
    std::string m_currentClip;
    
    // State management
    PropertyStateManager m_stateManager;
    PropertyReflectionSystem m_reflectionSystem;
    SceneSnapshot m_initialState;
    SceneSnapshot m_lastKnownState;
    
    // Batch recording
    std::vector<PropertyChangeEvent> m_batchedChanges;
    
    // Callbacks
    KeyframeRecordedCallback m_keyframeRecordedCallback;
    
    // Internal methods
    bool ShouldRecordProperty(Node* node, const std::string& propertyName) const;
    void ProcessPropertyChange(const PropertyChangeEvent& change);
    void CreateKeyframe(const std::string& nodePath, const std::string& propertyName, 
                       const EnhancedAnimationValue& value, float time);
    
    // Property monitoring helpers
    void UpdateLastKnownState();
    void DetectChanges();
    
    // Animation resource helpers
    AnimationTrack* FindOrCreateTrack(const std::string& nodePath, const std::string& propertyName);
    AnimationPropertyType ConvertToAnimationPropertyType(ExportVariableType exportType) const;
    AnimationValue ConvertToAnimationValue(const EnhancedAnimationValue& enhancedValue) const;
};

/**
 * @brief Property change monitor for automatic detection
 */
class PropertyChangeMonitor {
public:
    PropertyChangeMonitor(AutokeySystem* autokeySystem);
    ~PropertyChangeMonitor();
    
    // Monitoring control
    void StartMonitoring(Scene* scene);
    void StopMonitoring();
    bool IsMonitoring() const { return m_isMonitoring; }
    
    // Update cycle
    void Update(float deltaTime);
    
    // Configuration
    void SetUpdateInterval(float interval) { m_updateInterval = interval; }
    float GetUpdateInterval() const { return m_updateInterval; }
    
private:
    AutokeySystem* m_autokeySystem;
    Scene* m_scene;
    bool m_isMonitoring;
    float m_updateInterval;
    float m_timeSinceLastUpdate;
    
    PropertyStateManager m_stateManager;
    SceneSnapshot m_lastSnapshot;
    
    void CaptureCurrentState();
    void CompareAndReportChanges(const SceneSnapshot& newSnapshot);
};

/**
 * @brief Undo/Redo system for autokey operations
 */
class AutokeyUndoSystem {
public:
    struct UndoAction {
        enum Type {
            KeyframeAdded,
            KeyframeRemoved,
            KeyframeModified,
            TrackAdded,
            TrackRemoved
        };
        
        Type type;
        std::string nodePath;
        std::string propertyName;
        float time;
        AnimationValue oldValue;
        AnimationValue newValue;
        std::string description;
        
        UndoAction() = default;
        UndoAction(Type t, const std::string& path, const std::string& prop, float tm, 
                  const std::string& desc = "")
            : type(t), nodePath(path), propertyName(prop), time(tm), description(desc) {}
    };
    
    AutokeyUndoSystem(TweenAnimationResource* resource);
    ~AutokeyUndoSystem();
    
    // Undo/Redo operations
    bool CanUndo() const { return !m_undoStack.empty(); }
    bool CanRedo() const { return !m_redoStack.empty(); }
    
    void Undo();
    void Redo();
    void Clear();
    
    // Action recording
    void RecordAction(const UndoAction& action);
    void BeginActionGroup(const std::string& description);
    void EndActionGroup();
    
    // Configuration
    void SetMaxUndoLevels(size_t maxLevels) { m_maxUndoLevels = maxLevels; }
    size_t GetMaxUndoLevels() const { return m_maxUndoLevels; }
    
private:
    TweenAnimationResource* m_animationResource;
    std::vector<std::vector<UndoAction>> m_undoStack;
    std::vector<std::vector<UndoAction>> m_redoStack;
    std::vector<UndoAction> m_currentActionGroup;
    bool m_inActionGroup;
    size_t m_maxUndoLevels;
    
    void ExecuteAction(const UndoAction& action, bool isUndo);
    void TrimUndoStack();
};

/**
 * @brief Enhanced keyframe management with autokey support
 */
class EnhancedKeyframeManager {
public:
    EnhancedKeyframeManager(TweenAnimationResource* resource);
    ~EnhancedKeyframeManager();
    
    // Keyframe operations
    bool AddKeyframe(const std::string& clipName, const std::string& nodePath, 
                    const std::string& propertyName, float time, const AnimationValue& value,
                    InterpolationType interpolation = InterpolationType::Linear);
    
    bool RemoveKeyframe(const std::string& clipName, const std::string& nodePath,
                       const std::string& propertyName, float time);
    
    bool ModifyKeyframe(const std::string& clipName, const std::string& nodePath,
                       const std::string& propertyName, float time, const AnimationValue& newValue);
    
    // Bulk operations
    void CopyKeyframes(const std::string& sourceClip, const std::string& targetClip,
                      float sourceTime, float targetTime, const std::vector<std::string>& propertyNames = {});
    
    void PasteKeyframes(const std::string& clipName, float targetTime,
                       const std::vector<std::string>& propertyNames = {});
    
    void DeleteKeyframes(const std::string& clipName, float time,
                        const std::vector<std::string>& propertyNames = {});
    
    // Selection management
    void SelectKeyframe(const std::string& nodePath, const std::string& propertyName, float time);
    void DeselectKeyframe(const std::string& nodePath, const std::string& propertyName, float time);
    void ClearSelection();
    bool IsKeyframeSelected(const std::string& nodePath, const std::string& propertyName, float time) const;
    
    // Query operations
    std::vector<float> GetKeyframeTimes(const std::string& clipName, const std::string& nodePath,
                                       const std::string& propertyName) const;
    
    AnimationValue GetKeyframeValue(const std::string& clipName, const std::string& nodePath,
                                   const std::string& propertyName, float time) const;
    
    bool HasKeyframe(const std::string& clipName, const std::string& nodePath,
                    const std::string& propertyName, float time, float tolerance = 0.001f) const;
    
    // Autokey integration
    void SetAutokeySystem(AutokeySystem* autokeySystem) { m_autokeySystem = autokeySystem; }
    
private:
    TweenAnimationResource* m_animationResource;
    AutokeySystem* m_autokeySystem;
    
    struct KeyframeSelection {
        std::string nodePath;
        std::string propertyName;
        float time;
        
        bool operator<(const KeyframeSelection& other) const {
            if (nodePath != other.nodePath) return nodePath < other.nodePath;
            if (propertyName != other.propertyName) return propertyName < other.propertyName;
            return time < other.time;
        }
    };
    
    std::set<KeyframeSelection> m_selectedKeyframes;
    std::vector<AnimationKeyframe> m_clipboard;
    
    AnimationTrack* FindTrack(const std::string& clipName, const std::string& nodePath,
                             const std::string& propertyName) const;
    
    AnimationKeyframe* FindKeyframe(AnimationTrack* track, float time, float tolerance = 0.001f) const;
};

} // namespace Lupine
