#pragma once

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QColorDialog>
#include <QFileDialog>
#include <QComboBox>
#include <QFontDatabase>
#include "lupine/core/Component.h"
#include "lupine/resources/ResourceManager.h"

namespace Lupine {
    class Scene;
}

/**
 * @brief Base class for property editor widgets
 */
class PropertyEditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PropertyEditorWidget(const QString& name, const QString& description, QWidget* parent = nullptr);
    virtual ~PropertyEditorWidget() = default;

    virtual void setValue(const Lupine::ExportValue& value) = 0;
    virtual Lupine::ExportValue getValue() const = 0;

    void setDefaultValue(const Lupine::ExportValue& defaultValue);

signals:
    void valueChanged(const Lupine::ExportValue& value);
    void resetRequested();

protected slots:
    void onResetClicked();

protected:
    QLabel* m_nameLabel;
    QHBoxLayout* m_layout;
    QPushButton* m_resetButton;
    Lupine::ExportValue m_defaultValue;
};

/**
 * @brief Boolean property editor with checkbox
 */
class BoolPropertyEditor : public PropertyEditorWidget
{
    Q_OBJECT

public:
    explicit BoolPropertyEditor(const QString& name, const QString& description, QWidget* parent = nullptr);
    
    void setValue(const Lupine::ExportValue& value) override;
    Lupine::ExportValue getValue() const override;

private slots:
    void onCheckBoxToggled(bool checked);

private:
    QCheckBox* m_checkBox;
};

/**
 * @brief Integer property editor with spin box
 */
class IntPropertyEditor : public PropertyEditorWidget
{
    Q_OBJECT

public:
    explicit IntPropertyEditor(const QString& name, const QString& description, QWidget* parent = nullptr);

    void setValue(const Lupine::ExportValue& value) override;
    Lupine::ExportValue getValue() const override;

private slots:
    void onSpinBoxValueChanged(int value);

private:
    QSpinBox* m_spinBox;
};

/**
 * @brief Enum property editor with combo box
 */
class EnumPropertyEditor : public PropertyEditorWidget
{
    Q_OBJECT

public:
    explicit EnumPropertyEditor(const QString& name, const QString& description, const QStringList& options, QWidget* parent = nullptr);

    void setValue(const Lupine::ExportValue& value) override;
    Lupine::ExportValue getValue() const override;

private slots:
    void onComboBoxCurrentIndexChanged(int index);

private:
    QComboBox* m_comboBox;
};

/**
 * @brief Float property editor with double spin box
 */
class FloatPropertyEditor : public PropertyEditorWidget
{
    Q_OBJECT

public:
    explicit FloatPropertyEditor(const QString& name, const QString& description, QWidget* parent = nullptr);
    
    void setValue(const Lupine::ExportValue& value) override;
    Lupine::ExportValue getValue() const override;

private slots:
    void onSpinBoxValueChanged(double value);

private:
    QDoubleSpinBox* m_spinBox;
};

/**
 * @brief String property editor with line edit
 */
class StringPropertyEditor : public PropertyEditorWidget
{
    Q_OBJECT

public:
    explicit StringPropertyEditor(const QString& name, const QString& description, QWidget* parent = nullptr);
    
    void setValue(const Lupine::ExportValue& value) override;
    Lupine::ExportValue getValue() const override;

private slots:
    void onLineEditTextChanged(const QString& text);

private:
    QLineEdit* m_lineEdit;
};

/**
 * @brief File path property editor with browse button
 */
class FilePathPropertyEditor : public PropertyEditorWidget
{
    Q_OBJECT

public:
    explicit FilePathPropertyEditor(const QString& name, const QString& description, QWidget* parent = nullptr);

    void setValue(const Lupine::ExportValue& value) override;
    Lupine::ExportValue getValue() const override;

private slots:
    void onLineEditTextChanged(const QString& text);
    void onBrowseButtonClicked();

private:
    QLineEdit* m_lineEdit;
    QPushButton* m_browseButton;
};

/**
 * @brief Font path property editor with system font dropdown and file browser
 */
class FontPathPropertyEditor : public PropertyEditorWidget
{
    Q_OBJECT

public:
    explicit FontPathPropertyEditor(const QString& name, const QString& description, QWidget* parent = nullptr);

    void setValue(const Lupine::ExportValue& value) override;
    Lupine::ExportValue getValue() const override;

private slots:
    void onComboBoxCurrentIndexChanged(int index);
    void onBrowseButtonClicked();
    void onUseSystemFontToggled(bool checked);

private:
    void populateSystemFonts();
    void updateUI();
    void updatePreview();

    QCheckBox* m_useSystemFontCheckBox;
    QComboBox* m_systemFontComboBox;
    QLineEdit* m_filePathLineEdit;
    QPushButton* m_browseButton;
    QLabel* m_previewLabel;

    Lupine::FontPath m_currentFontPath;
};

/**
 * @brief Color property editor with color picker
 */
class ColorPropertyEditor : public PropertyEditorWidget
{
    Q_OBJECT

public:
    explicit ColorPropertyEditor(const QString& name, const QString& description, QWidget* parent = nullptr);
    
    void setValue(const Lupine::ExportValue& value) override;
    Lupine::ExportValue getValue() const override;

private slots:
    void onColorButtonClicked();

private:
    QPushButton* m_colorButton;
    QColor m_currentColor;
    
    void updateColorButton();
};

/**
 * @brief Vector2 property editor with two spin boxes
 */
class Vec2PropertyEditor : public PropertyEditorWidget
{
    Q_OBJECT

public:
    explicit Vec2PropertyEditor(const QString& name, const QString& description, QWidget* parent = nullptr);
    
    void setValue(const Lupine::ExportValue& value) override;
    Lupine::ExportValue getValue() const override;

private slots:
    void onSpinBoxValueChanged();

private:
    QDoubleSpinBox* m_xSpinBox;
    QDoubleSpinBox* m_ySpinBox;
};

/**
 * @brief Vector3 property editor with three spin boxes
 */
class Vec3PropertyEditor : public PropertyEditorWidget
{
    Q_OBJECT

public:
    explicit Vec3PropertyEditor(const QString& name, const QString& description, QWidget* parent = nullptr);
    
    void setValue(const Lupine::ExportValue& value) override;
    Lupine::ExportValue getValue() const override;

private slots:
    void onSpinBoxValueChanged();

private:
    QDoubleSpinBox* m_xSpinBox;
    QDoubleSpinBox* m_ySpinBox;
    QDoubleSpinBox* m_zSpinBox;
};

/**
 * @brief Vector4 property editor with four spin boxes
 */
class Vec4PropertyEditor : public PropertyEditorWidget
{
    Q_OBJECT

public:
    explicit Vec4PropertyEditor(const QString& name, const QString& description, QWidget* parent = nullptr);
    
    void setValue(const Lupine::ExportValue& value) override;
    Lupine::ExportValue getValue() const override;

private slots:
    void onSpinBoxValueChanged();

private:
    QDoubleSpinBox* m_xSpinBox;
    QDoubleSpinBox* m_ySpinBox;
    QDoubleSpinBox* m_zSpinBox;
    QDoubleSpinBox* m_wSpinBox;
};

/**
 * @brief Node reference property editor with selection button
 */
class NodeReferencePropertyEditor : public PropertyEditorWidget
{
    Q_OBJECT

public:
    explicit NodeReferencePropertyEditor(const QString& name, const QString& description, QWidget* parent = nullptr);

    void setValue(const Lupine::ExportValue& value) override;
    Lupine::ExportValue getValue() const override;

    /**
     * @brief Set the scene to use for node selection
     * @param scene Scene containing the nodes
     */
    void setScene(Lupine::Scene* scene);

private slots:
    void onSelectButtonClicked();
    void onClearButtonClicked();

private:
    void updateNodeNameDisplay();
    Lupine::Node* findNodeByUUID(const Lupine::UUID& uuid) const;

    QLineEdit* m_nodeNameEdit;
    QPushButton* m_selectButton;
    QPushButton* m_clearButton;
    Lupine::UUID m_nodeUUID;
    Lupine::Scene* m_scene;
};

/**
 * @brief Factory function for creating property editors based on type
 */
PropertyEditorWidget* CreatePropertyEditor(const QString& name, const QString& description,
                                         Lupine::ExportVariableType type, QWidget* parent = nullptr);

/**
 * @brief Factory function for creating property editors with enum options
 */
PropertyEditorWidget* CreatePropertyEditor(const QString& name, const QString& description,
                                         Lupine::ExportVariableType type, const QStringList& enumOptions, QWidget* parent = nullptr);
