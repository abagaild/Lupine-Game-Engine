#include "PropertyEditorWidget.h"
#include "../dialogs/NodeSelectionDialog.h"
#include "lupine/core/Scene.h"
#include "lupine/core/Node.h"
#include <QApplication>
#include <QStyle>
#include <glm/glm.hpp>

// PropertyEditorWidget base class
PropertyEditorWidget::PropertyEditorWidget(const QString& name, const QString& description, QWidget* parent)
    : QWidget(parent)
{
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(4, 2, 4, 2);
    m_layout->setSpacing(4);

    m_nameLabel = new QLabel(name);
    m_nameLabel->setMinimumWidth(80);  // Reduced minimum width
    m_nameLabel->setMaximumWidth(120); // Set maximum width to prevent excessive expansion
    m_nameLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_nameLabel->setToolTip(description);
    m_nameLabel->setWordWrap(false);
    m_nameLabel->setStyleSheet("font-weight: bold; color: #ddd;");
    m_layout->addWidget(m_nameLabel);

    // Add reset button at the end (will be added after the property widget)
    m_resetButton = new QPushButton();
    m_resetButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_BrowserReload));
    m_resetButton->setToolTip("Reset to default value");
    m_resetButton->setMaximumSize(20, 20);
    m_resetButton->setMinimumSize(20, 20);
    m_resetButton->setFlat(true);
    m_resetButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(m_resetButton, &QPushButton::clicked, this, &PropertyEditorWidget::onResetClicked);
}

void PropertyEditorWidget::setDefaultValue(const Lupine::ExportValue& defaultValue) {
    m_defaultValue = defaultValue;
}

void PropertyEditorWidget::onResetClicked() {
    setValue(m_defaultValue);
    emit resetRequested();
}

// BoolPropertyEditor
BoolPropertyEditor::BoolPropertyEditor(const QString& name, const QString& description, QWidget* parent)
    : PropertyEditorWidget(name, description, parent)
{
    m_checkBox = new QCheckBox();
    m_layout->addWidget(m_checkBox);
    m_layout->addStretch();
    m_layout->addWidget(m_resetButton);

    connect(m_checkBox, &QCheckBox::toggled, this, &BoolPropertyEditor::onCheckBoxToggled);
}

void BoolPropertyEditor::setValue(const Lupine::ExportValue& value) {
    if (std::holds_alternative<bool>(value)) {
        m_checkBox->setChecked(std::get<bool>(value));
    }
}

Lupine::ExportValue BoolPropertyEditor::getValue() const {
    return m_checkBox->isChecked();
}

void BoolPropertyEditor::onCheckBoxToggled(bool checked) {
    emit valueChanged(checked);
}

// IntPropertyEditor
IntPropertyEditor::IntPropertyEditor(const QString& name, const QString& description, QWidget* parent)
    : PropertyEditorWidget(name, description, parent)
{
    m_spinBox = new QSpinBox();
    m_spinBox->setRange(-999999, 999999);
    m_spinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_spinBox->setMinimumWidth(60);
    m_layout->addWidget(m_spinBox);
    m_layout->addWidget(m_resetButton);

    connect(m_spinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &IntPropertyEditor::onSpinBoxValueChanged);
}

void IntPropertyEditor::setValue(const Lupine::ExportValue& value) {
    if (std::holds_alternative<int>(value)) {
        m_spinBox->setValue(std::get<int>(value));
    }
}

Lupine::ExportValue IntPropertyEditor::getValue() const {
    return m_spinBox->value();
}

void IntPropertyEditor::onSpinBoxValueChanged(int value) {
    emit valueChanged(value);
}

// EnumPropertyEditor
EnumPropertyEditor::EnumPropertyEditor(const QString& name, const QString& description, const QStringList& options, QWidget* parent)
    : PropertyEditorWidget(name, description, parent)
{
    m_comboBox = new QComboBox();
    m_comboBox->addItems(options);
    m_layout->addWidget(m_comboBox);
    m_layout->addStretch();
    m_layout->addWidget(m_resetButton);

    connect(m_comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &EnumPropertyEditor::onComboBoxCurrentIndexChanged);
}

void EnumPropertyEditor::setValue(const Lupine::ExportValue& value) {
    if (std::holds_alternative<int>(value)) {
        int index = std::get<int>(value);
        if (index >= 0 && index < m_comboBox->count()) {
            m_comboBox->setCurrentIndex(index);
        }
    }
}

Lupine::ExportValue EnumPropertyEditor::getValue() const {
    return m_comboBox->currentIndex();
}

void EnumPropertyEditor::onComboBoxCurrentIndexChanged(int index) {
    emit valueChanged(index);
}

// FloatPropertyEditor
FloatPropertyEditor::FloatPropertyEditor(const QString& name, const QString& description, QWidget* parent)
    : PropertyEditorWidget(name, description, parent)
{
    m_spinBox = new QDoubleSpinBox();
    m_spinBox->setRange(-999999.0, 999999.0);
    m_spinBox->setDecimals(3);
    m_spinBox->setSingleStep(0.1);
    m_spinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_spinBox->setMinimumWidth(60);
    m_layout->addWidget(m_spinBox);
    m_layout->addWidget(m_resetButton);

    connect(m_spinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &FloatPropertyEditor::onSpinBoxValueChanged);
}

void FloatPropertyEditor::setValue(const Lupine::ExportValue& value) {
    if (std::holds_alternative<float>(value)) {
        m_spinBox->setValue(static_cast<double>(std::get<float>(value)));
    }
}

Lupine::ExportValue FloatPropertyEditor::getValue() const {
    return static_cast<float>(m_spinBox->value());
}

void FloatPropertyEditor::onSpinBoxValueChanged(double value) {
    emit valueChanged(static_cast<float>(value));
}

// StringPropertyEditor
StringPropertyEditor::StringPropertyEditor(const QString& name, const QString& description, QWidget* parent)
    : PropertyEditorWidget(name, description, parent)
{
    m_lineEdit = new QLineEdit();
    m_lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_lineEdit->setMinimumWidth(80);
    m_layout->addWidget(m_lineEdit);
    m_layout->addWidget(m_resetButton);

    connect(m_lineEdit, &QLineEdit::textChanged, this, &StringPropertyEditor::onLineEditTextChanged);
}

void StringPropertyEditor::setValue(const Lupine::ExportValue& value) {
    if (std::holds_alternative<std::string>(value)) {
        m_lineEdit->setText(QString::fromStdString(std::get<std::string>(value)));
    }
}

Lupine::ExportValue StringPropertyEditor::getValue() const {
    return m_lineEdit->text().toStdString();
}

void StringPropertyEditor::onLineEditTextChanged(const QString& text) {
    emit valueChanged(text.toStdString());
}

// FilePathPropertyEditor
FilePathPropertyEditor::FilePathPropertyEditor(const QString& name, const QString& description, QWidget* parent)
    : PropertyEditorWidget(name, description, parent)
{
    m_lineEdit = new QLineEdit();
    m_browseButton = new QPushButton("...");
    m_browseButton->setMaximumWidth(30);

    m_layout->addWidget(m_lineEdit);
    m_layout->addWidget(m_browseButton);
    m_layout->addWidget(m_resetButton);

    connect(m_lineEdit, &QLineEdit::textChanged, this, &FilePathPropertyEditor::onLineEditTextChanged);
    connect(m_browseButton, &QPushButton::clicked, this, &FilePathPropertyEditor::onBrowseButtonClicked);
}

void FilePathPropertyEditor::setValue(const Lupine::ExportValue& value) {
    if (std::holds_alternative<std::string>(value)) {
        m_lineEdit->setText(QString::fromStdString(std::get<std::string>(value)));
    }
}

Lupine::ExportValue FilePathPropertyEditor::getValue() const {
    return m_lineEdit->text().toStdString();
}

void FilePathPropertyEditor::onLineEditTextChanged(const QString& text) {
    emit valueChanged(text.toStdString());
}

void FilePathPropertyEditor::onBrowseButtonClicked() {
    QString fileName = QFileDialog::getOpenFileName(this, "Select File", m_lineEdit->text());
    if (!fileName.isEmpty()) {
        m_lineEdit->setText(fileName);
    }
}

// ColorPropertyEditor
ColorPropertyEditor::ColorPropertyEditor(const QString& name, const QString& description, QWidget* parent)
    : PropertyEditorWidget(name, description, parent)
    , m_currentColor(Qt::white)
{
    m_colorButton = new QPushButton();
    m_colorButton->setMaximumWidth(60);
    m_colorButton->setMaximumHeight(25);
    updateColorButton();

    m_layout->addWidget(m_colorButton);
    m_layout->addStretch();
    m_layout->addWidget(m_resetButton);

    connect(m_colorButton, &QPushButton::clicked, this, &ColorPropertyEditor::onColorButtonClicked);
}

void ColorPropertyEditor::setValue(const Lupine::ExportValue& value) {
    if (std::holds_alternative<glm::vec4>(value)) {
        glm::vec4 color = std::get<glm::vec4>(value);
        m_currentColor = QColor::fromRgbF(color.r, color.g, color.b, color.a);
        updateColorButton();
    }
}

Lupine::ExportValue ColorPropertyEditor::getValue() const {
    return glm::vec4(
        static_cast<float>(m_currentColor.redF()),
        static_cast<float>(m_currentColor.greenF()),
        static_cast<float>(m_currentColor.blueF()),
        static_cast<float>(m_currentColor.alphaF())
    );
}

void ColorPropertyEditor::onColorButtonClicked() {
    QColor newColor = QColorDialog::getColor(m_currentColor, this, "Select Color", QColorDialog::ShowAlphaChannel);
    if (newColor.isValid()) {
        m_currentColor = newColor;
        updateColorButton();
        emit valueChanged(getValue());
    }
}

void ColorPropertyEditor::updateColorButton() {
    QString style = QString("background-color: rgba(%1, %2, %3, %4); border: 1px solid black;")
                    .arg(m_currentColor.red())
                    .arg(m_currentColor.green())
                    .arg(m_currentColor.blue())
                    .arg(m_currentColor.alpha());
    m_colorButton->setStyleSheet(style);
}

// Vec2PropertyEditor
Vec2PropertyEditor::Vec2PropertyEditor(const QString& name, const QString& description, QWidget* parent)
    : PropertyEditorWidget(name, description, parent)
{
    m_xSpinBox = new QDoubleSpinBox();
    m_ySpinBox = new QDoubleSpinBox();

    m_xSpinBox->setRange(-999999.0, 999999.0);
    m_ySpinBox->setRange(-999999.0, 999999.0);
    m_xSpinBox->setDecimals(3);
    m_ySpinBox->setDecimals(3);
    m_xSpinBox->setSingleStep(0.1);
    m_ySpinBox->setSingleStep(0.1);

    // Set size policies for responsive behavior
    m_xSpinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_ySpinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_xSpinBox->setMinimumWidth(50);
    m_ySpinBox->setMinimumWidth(50);

    // Create a container widget for the vector components
    QWidget* vectorContainer = new QWidget();
    QHBoxLayout* vectorLayout = new QHBoxLayout(vectorContainer);
    vectorLayout->setContentsMargins(0, 0, 0, 0);
    vectorLayout->setSpacing(4);

    QLabel* xLabel = new QLabel("X:");
    QLabel* yLabel = new QLabel("Y:");

    // Style the labels with color coding
    xLabel->setStyleSheet("color: #f88; font-size: 10px;");
    yLabel->setStyleSheet("color: #8f8; font-size: 10px;");

    xLabel->setMinimumWidth(12);
    yLabel->setMinimumWidth(12);
    xLabel->setMaximumWidth(12);
    yLabel->setMaximumWidth(12);

    vectorLayout->addWidget(xLabel);
    vectorLayout->addWidget(m_xSpinBox);
    vectorLayout->addWidget(yLabel);
    vectorLayout->addWidget(m_ySpinBox);

    m_layout->addWidget(vectorContainer);
    m_layout->addWidget(m_resetButton);

    connect(m_xSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Vec2PropertyEditor::onSpinBoxValueChanged);
    connect(m_ySpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Vec2PropertyEditor::onSpinBoxValueChanged);
}

void Vec2PropertyEditor::setValue(const Lupine::ExportValue& value) {
    if (std::holds_alternative<glm::vec2>(value)) {
        glm::vec2 vec = std::get<glm::vec2>(value);
        m_xSpinBox->setValue(vec.x);
        m_ySpinBox->setValue(vec.y);
    }
}

Lupine::ExportValue Vec2PropertyEditor::getValue() const {
    return glm::vec2(
        static_cast<float>(m_xSpinBox->value()),
        static_cast<float>(m_ySpinBox->value())
    );
}

void Vec2PropertyEditor::onSpinBoxValueChanged() {
    emit valueChanged(getValue());
}

// Vec3PropertyEditor
Vec3PropertyEditor::Vec3PropertyEditor(const QString& name, const QString& description, QWidget* parent)
    : PropertyEditorWidget(name, description, parent)
{
    m_xSpinBox = new QDoubleSpinBox();
    m_ySpinBox = new QDoubleSpinBox();
    m_zSpinBox = new QDoubleSpinBox();

    m_xSpinBox->setRange(-999999.0, 999999.0);
    m_ySpinBox->setRange(-999999.0, 999999.0);
    m_zSpinBox->setRange(-999999.0, 999999.0);
    m_xSpinBox->setDecimals(3);
    m_ySpinBox->setDecimals(3);
    m_zSpinBox->setDecimals(3);
    m_xSpinBox->setSingleStep(0.1);
    m_ySpinBox->setSingleStep(0.1);
    m_zSpinBox->setSingleStep(0.1);

    // Set size policies for responsive behavior
    m_xSpinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_ySpinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_zSpinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_xSpinBox->setMinimumWidth(40);
    m_ySpinBox->setMinimumWidth(40);
    m_zSpinBox->setMinimumWidth(40);

    // Create a container widget for the vector components
    QWidget* vectorContainer = new QWidget();
    QHBoxLayout* vectorLayout = new QHBoxLayout(vectorContainer);
    vectorLayout->setContentsMargins(0, 0, 0, 0);
    vectorLayout->setSpacing(2);

    QLabel* xLabel = new QLabel("X:");
    QLabel* yLabel = new QLabel("Y:");
    QLabel* zLabel = new QLabel("Z:");

    // Style the labels with color coding
    xLabel->setStyleSheet("color: #f88; font-size: 10px;");
    yLabel->setStyleSheet("color: #8f8; font-size: 10px;");
    zLabel->setStyleSheet("color: #88f; font-size: 10px;");

    xLabel->setMinimumWidth(12);
    yLabel->setMinimumWidth(12);
    zLabel->setMinimumWidth(12);
    xLabel->setMaximumWidth(12);
    yLabel->setMaximumWidth(12);
    zLabel->setMaximumWidth(12);

    vectorLayout->addWidget(xLabel);
    vectorLayout->addWidget(m_xSpinBox);
    vectorLayout->addWidget(yLabel);
    vectorLayout->addWidget(m_ySpinBox);
    vectorLayout->addWidget(zLabel);
    vectorLayout->addWidget(m_zSpinBox);

    m_layout->addWidget(vectorContainer);
    m_layout->addWidget(m_resetButton);

    connect(m_xSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Vec3PropertyEditor::onSpinBoxValueChanged);
    connect(m_ySpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Vec3PropertyEditor::onSpinBoxValueChanged);
    connect(m_zSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Vec3PropertyEditor::onSpinBoxValueChanged);
}

void Vec3PropertyEditor::setValue(const Lupine::ExportValue& value) {
    if (std::holds_alternative<glm::vec3>(value)) {
        glm::vec3 vec = std::get<glm::vec3>(value);
        m_xSpinBox->setValue(vec.x);
        m_ySpinBox->setValue(vec.y);
        m_zSpinBox->setValue(vec.z);
    }
}

Lupine::ExportValue Vec3PropertyEditor::getValue() const {
    return glm::vec3(
        static_cast<float>(m_xSpinBox->value()),
        static_cast<float>(m_ySpinBox->value()),
        static_cast<float>(m_zSpinBox->value())
    );
}

void Vec3PropertyEditor::onSpinBoxValueChanged() {
    emit valueChanged(getValue());
}

// Vec4PropertyEditor
Vec4PropertyEditor::Vec4PropertyEditor(const QString& name, const QString& description, QWidget* parent)
    : PropertyEditorWidget(name, description, parent)
{
    m_xSpinBox = new QDoubleSpinBox();
    m_ySpinBox = new QDoubleSpinBox();
    m_zSpinBox = new QDoubleSpinBox();
    m_wSpinBox = new QDoubleSpinBox();

    m_xSpinBox->setRange(-999999.0, 999999.0);
    m_ySpinBox->setRange(-999999.0, 999999.0);
    m_zSpinBox->setRange(-999999.0, 999999.0);
    m_wSpinBox->setRange(-999999.0, 999999.0);
    m_xSpinBox->setDecimals(3);
    m_ySpinBox->setDecimals(3);
    m_zSpinBox->setDecimals(3);
    m_wSpinBox->setDecimals(3);
    m_xSpinBox->setSingleStep(0.1);
    m_ySpinBox->setSingleStep(0.1);
    m_zSpinBox->setSingleStep(0.1);
    m_wSpinBox->setSingleStep(0.1);

    // Set size policies for responsive behavior
    m_xSpinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_ySpinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_zSpinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_wSpinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_xSpinBox->setMinimumWidth(40);
    m_ySpinBox->setMinimumWidth(40);
    m_zSpinBox->setMinimumWidth(40);
    m_wSpinBox->setMinimumWidth(40);

    // Create a container widget for the vector components
    QWidget* vectorContainer = new QWidget();
    QHBoxLayout* vectorLayout = new QHBoxLayout(vectorContainer);
    vectorLayout->setContentsMargins(0, 0, 0, 0);
    vectorLayout->setSpacing(2);

    QLabel* xLabel = new QLabel("X:");
    QLabel* yLabel = new QLabel("Y:");
    QLabel* zLabel = new QLabel("Z:");
    QLabel* wLabel = new QLabel("W:");

    // Style the labels with color coding
    xLabel->setStyleSheet("color: #f88; font-size: 10px;");
    yLabel->setStyleSheet("color: #8f8; font-size: 10px;");
    zLabel->setStyleSheet("color: #88f; font-size: 10px;");
    wLabel->setStyleSheet("color: #ff8; font-size: 10px;");

    xLabel->setMinimumWidth(12);
    yLabel->setMinimumWidth(12);
    zLabel->setMinimumWidth(12);
    wLabel->setMinimumWidth(12);
    xLabel->setMaximumWidth(12);
    yLabel->setMaximumWidth(12);
    zLabel->setMaximumWidth(12);
    wLabel->setMaximumWidth(12);

    vectorLayout->addWidget(xLabel);
    vectorLayout->addWidget(m_xSpinBox);
    vectorLayout->addWidget(yLabel);
    vectorLayout->addWidget(m_ySpinBox);
    vectorLayout->addWidget(zLabel);
    vectorLayout->addWidget(m_zSpinBox);
    vectorLayout->addWidget(wLabel);
    vectorLayout->addWidget(m_wSpinBox);

    m_layout->addWidget(vectorContainer);
    m_layout->addWidget(m_resetButton);

    connect(m_xSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Vec4PropertyEditor::onSpinBoxValueChanged);
    connect(m_ySpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Vec4PropertyEditor::onSpinBoxValueChanged);
    connect(m_zSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Vec4PropertyEditor::onSpinBoxValueChanged);
    connect(m_wSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Vec4PropertyEditor::onSpinBoxValueChanged);
}

void Vec4PropertyEditor::setValue(const Lupine::ExportValue& value) {
    if (std::holds_alternative<glm::vec4>(value)) {
        glm::vec4 vec = std::get<glm::vec4>(value);
        m_xSpinBox->setValue(vec.x);
        m_ySpinBox->setValue(vec.y);
        m_zSpinBox->setValue(vec.z);
        m_wSpinBox->setValue(vec.w);
    }
}

Lupine::ExportValue Vec4PropertyEditor::getValue() const {
    return glm::vec4(
        static_cast<float>(m_xSpinBox->value()),
        static_cast<float>(m_ySpinBox->value()),
        static_cast<float>(m_zSpinBox->value()),
        static_cast<float>(m_wSpinBox->value())
    );
}

void Vec4PropertyEditor::onSpinBoxValueChanged() {
    emit valueChanged(getValue());
}

// NodeReferencePropertyEditor
NodeReferencePropertyEditor::NodeReferencePropertyEditor(const QString& name, const QString& description, QWidget* parent)
    : PropertyEditorWidget(name, description, parent)
    , m_scene(nullptr)
{
    m_nodeNameEdit = new QLineEdit();
    m_nodeNameEdit->setReadOnly(true);
    m_nodeNameEdit->setPlaceholderText("No node selected");

    m_selectButton = new QPushButton("Select");
    m_clearButton = new QPushButton("Clear");
    m_selectButton->setMaximumWidth(60);
    m_clearButton->setMaximumWidth(50);

    m_layout->addWidget(m_nodeNameEdit);
    m_layout->addWidget(m_selectButton);
    m_layout->addWidget(m_clearButton);
    m_layout->addWidget(m_resetButton);

    connect(m_selectButton, &QPushButton::clicked, this, &NodeReferencePropertyEditor::onSelectButtonClicked);
    connect(m_clearButton, &QPushButton::clicked, this, &NodeReferencePropertyEditor::onClearButtonClicked);
}

void NodeReferencePropertyEditor::setValue(const Lupine::ExportValue& value) {
    if (std::holds_alternative<Lupine::UUID>(value)) {
        m_nodeUUID = std::get<Lupine::UUID>(value);
        updateNodeNameDisplay();
    } else {
        m_nodeUUID = Lupine::UUID();
        m_nodeNameEdit->clear();
    }
}

Lupine::ExportValue NodeReferencePropertyEditor::getValue() const {
    return m_nodeUUID;
}

void NodeReferencePropertyEditor::setScene(Lupine::Scene* scene) {
    m_scene = scene;
    updateNodeNameDisplay(); // Update display in case we already have a UUID
}

void NodeReferencePropertyEditor::onSelectButtonClicked() {
    if (!m_scene) {
        qWarning() << "NodeReferencePropertyEditor: No scene set for node selection";
        return;
    }

    NodeSelectionDialog dialog(m_scene, this);
    if (!m_nodeUUID.IsNil()) {
        dialog.setSelectedNode(m_nodeUUID);
    }

    if (dialog.exec() == QDialog::Accepted) {
        if (auto* selectedNode = dialog.getSelectedNode()) {
            m_nodeUUID = selectedNode->GetUUID();
            updateNodeNameDisplay();
            emit valueChanged(m_nodeUUID);
        }
    }
}

void NodeReferencePropertyEditor::onClearButtonClicked() {
    m_nodeUUID = Lupine::UUID();
    m_nodeNameEdit->clear();
    emit valueChanged(m_nodeUUID);
}

void NodeReferencePropertyEditor::updateNodeNameDisplay() {
    if (m_nodeUUID.IsNil()) {
        m_nodeNameEdit->clear();
        return;
    }

    if (auto* node = findNodeByUUID(m_nodeUUID)) {
        m_nodeNameEdit->setText(QString::fromStdString(node->GetName()));
    } else {
        m_nodeNameEdit->setText("Node (UUID: " + QString::fromStdString(m_nodeUUID.ToString()) + ")");
    }
}

Lupine::Node* NodeReferencePropertyEditor::findNodeByUUID(const Lupine::UUID& uuid) const {
    if (!m_scene || uuid.IsNil()) return nullptr;

    std::function<Lupine::Node*(Lupine::Node*)> searchNode = [&](Lupine::Node* node) -> Lupine::Node* {
        if (node->GetUUID() == uuid) {
            return node;
        }

        for (const auto& child : node->GetChildren()) {
            if (auto* found = searchNode(child.get())) {
                return found;
            }
        }
        return nullptr;
    };

    if (auto* rootNode = m_scene->GetRootNode()) {
        return searchNode(rootNode);
    }
    return nullptr;
}

// Factory function
PropertyEditorWidget* CreatePropertyEditor(const QString& name, const QString& description,
                                         Lupine::ExportVariableType type, QWidget* parent) {
    switch (type) {
        case Lupine::ExportVariableType::Bool:
            return new BoolPropertyEditor(name, description, parent);
        case Lupine::ExportVariableType::Int:
            return new IntPropertyEditor(name, description, parent);
        case Lupine::ExportVariableType::Float:
            return new FloatPropertyEditor(name, description, parent);
        case Lupine::ExportVariableType::String:
            return new StringPropertyEditor(name, description, parent);
        case Lupine::ExportVariableType::FilePath:
            return new FilePathPropertyEditor(name, description, parent);
        case Lupine::ExportVariableType::FontPath:
            return new FontPathPropertyEditor(name, description, parent);
        case Lupine::ExportVariableType::Color:
            return new ColorPropertyEditor(name, description, parent);
        case Lupine::ExportVariableType::Vec2:
            return new Vec2PropertyEditor(name, description, parent);
        case Lupine::ExportVariableType::Vec3:
            return new Vec3PropertyEditor(name, description, parent);
        case Lupine::ExportVariableType::Vec4:
            return new Vec4PropertyEditor(name, description, parent);
        case Lupine::ExportVariableType::NodeReference:
            return new NodeReferencePropertyEditor(name, description, parent);
        case Lupine::ExportVariableType::Enum:
            // For enum without options, fall back to int editor
            return new IntPropertyEditor(name, description, parent);
        default:
            return new StringPropertyEditor(name, description, parent);
    }
}

// FontPathPropertyEditor
FontPathPropertyEditor::FontPathPropertyEditor(const QString& name, const QString& description, QWidget* parent)
    : PropertyEditorWidget(name, description, parent)
    , m_currentFontPath(Lupine::FontPath("Arial", true, "Regular"))
{
    // Create main layout
    QVBoxLayout* mainLayout = new QVBoxLayout();

    // System font checkbox
    m_useSystemFontCheckBox = new QCheckBox("Use System Font");
    m_useSystemFontCheckBox->setChecked(true);
    mainLayout->addWidget(m_useSystemFontCheckBox);

    // System font combo box
    m_systemFontComboBox = new QComboBox();
    populateSystemFonts();
    mainLayout->addWidget(m_systemFontComboBox);

    // File path section
    QHBoxLayout* fileLayout = new QHBoxLayout();
    m_filePathLineEdit = new QLineEdit();
    m_filePathLineEdit->setPlaceholderText("Select font file...");
    m_filePathLineEdit->setEnabled(false);
    m_browseButton = new QPushButton("Browse");
    m_browseButton->setEnabled(false);
    fileLayout->addWidget(m_filePathLineEdit);
    fileLayout->addWidget(m_browseButton);
    mainLayout->addLayout(fileLayout);

    // Preview label
    m_previewLabel = new QLabel("Preview: The quick brown fox jumps over the lazy dog");
    m_previewLabel->setStyleSheet("border: 1px solid gray; padding: 4px; background-color: white; color: black;");
    m_previewLabel->setMinimumHeight(30);
    mainLayout->addWidget(m_previewLabel);

    // Add to main widget layout
    m_layout->addLayout(mainLayout);
    m_layout->addStretch();
    m_layout->addWidget(m_resetButton);

    // Connect signals
    connect(m_useSystemFontCheckBox, &QCheckBox::toggled, this, &FontPathPropertyEditor::onUseSystemFontToggled);
    connect(m_systemFontComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &FontPathPropertyEditor::onComboBoxCurrentIndexChanged);
    connect(m_browseButton, &QPushButton::clicked, this, &FontPathPropertyEditor::onBrowseButtonClicked);
    connect(m_filePathLineEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        if (!m_useSystemFontCheckBox->isChecked()) {
            m_currentFontPath = Lupine::FontPath(text.toStdString(), false);
            updatePreview();
            emit valueChanged(m_currentFontPath);
        }
    });

    updateUI();
}

void FontPathPropertyEditor::setValue(const Lupine::ExportValue& value) {
    if (std::holds_alternative<Lupine::FontPath>(value)) {
        m_currentFontPath = std::get<Lupine::FontPath>(value);
        updateUI();
    }
}

Lupine::ExportValue FontPathPropertyEditor::getValue() const {
    return m_currentFontPath;
}

void FontPathPropertyEditor::onComboBoxCurrentIndexChanged(int index) {
    if (m_useSystemFontCheckBox->isChecked() && index >= 0) {
        QString fontName = m_systemFontComboBox->currentText();
        // Extract family and style from display name
        QStringList parts = fontName.split(" ");
        if (parts.size() >= 2 && (parts.last() == "Bold" || parts.last() == "Italic" || parts.last() == "Regular")) {
            QString style = parts.takeLast();
            QString family = parts.join(" ");
            m_currentFontPath = Lupine::FontPath(family.toStdString(), true, style.toStdString());
        } else {
            m_currentFontPath = Lupine::FontPath(fontName.toStdString(), true, "Regular");
        }
        updatePreview();
        emit valueChanged(m_currentFontPath);
    }
}

void FontPathPropertyEditor::onBrowseButtonClicked() {
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Select Font File",
        QString(),
        "Font Files (*.ttf *.otf *.ttc);;All Files (*)"
    );

    if (!fileName.isEmpty()) {
        m_filePathLineEdit->setText(fileName);
        m_currentFontPath = Lupine::FontPath(fileName.toStdString(), false);
        updatePreview();
        emit valueChanged(m_currentFontPath);
    }
}

void FontPathPropertyEditor::onUseSystemFontToggled(bool checked) {
    m_systemFontComboBox->setEnabled(checked);
    m_filePathLineEdit->setEnabled(!checked);
    m_browseButton->setEnabled(!checked);

    if (checked) {
        // Switch to system font
        onComboBoxCurrentIndexChanged(m_systemFontComboBox->currentIndex());
    } else {
        // Switch to file font
        QString filePath = m_filePathLineEdit->text();
        if (!filePath.isEmpty()) {
            m_currentFontPath = Lupine::FontPath(filePath.toStdString(), false);
            updatePreview();
            emit valueChanged(m_currentFontPath);
        }
    }
    updatePreview();
}

void FontPathPropertyEditor::populateSystemFonts() {
    m_systemFontComboBox->clear();

    // Get system fonts from ResourceManager
    auto systemFonts = Lupine::ResourceManager::EnumerateSystemFonts();

    // Add fonts to combo box
    QStringList fontNames;
    for (const auto& font : systemFonts) {
        QString displayName = QString::fromStdString(font.GetDisplayName());
        if (!fontNames.contains(displayName)) {
            fontNames.append(displayName);
            m_systemFontComboBox->addItem(displayName);
        }
    }

    // Set default selection
    int arialIndex = m_systemFontComboBox->findText("Arial");
    if (arialIndex >= 0) {
        m_systemFontComboBox->setCurrentIndex(arialIndex);
    }
}

void FontPathPropertyEditor::updateUI() {
    if (m_currentFontPath.is_system_font) {
        m_useSystemFontCheckBox->setChecked(true);
        QString displayName = QString::fromStdString(m_currentFontPath.GetDisplayName());
        int index = m_systemFontComboBox->findText(displayName);
        if (index >= 0) {
            m_systemFontComboBox->setCurrentIndex(index);
        }
        m_filePathLineEdit->clear();
    } else {
        m_useSystemFontCheckBox->setChecked(false);
        m_filePathLineEdit->setText(QString::fromStdString(m_currentFontPath.path));
    }

    updatePreview();
}

void FontPathPropertyEditor::updatePreview() {
    // Create a QFont from the current FontPath
    QFont previewFont;

    if (m_currentFontPath.is_system_font) {
        // Use system font
        previewFont.setFamily(QString::fromStdString(m_currentFontPath.path));

        // Set style
        if (m_currentFontPath.style_name == "Bold") {
            previewFont.setBold(true);
        } else if (m_currentFontPath.style_name == "Italic") {
            previewFont.setItalic(true);
        } else if (m_currentFontPath.style_name == "Bold Italic") {
            previewFont.setBold(true);
            previewFont.setItalic(true);
        }
    } else {
        // Load font from file
        if (!m_currentFontPath.path.empty()) {
            QString fontPath = QString::fromStdString(m_currentFontPath.path);
            int fontId = QFontDatabase::addApplicationFont(fontPath);
            if (fontId != -1) {
                QStringList fontFamilies = QFontDatabase::applicationFontFamilies(fontId);
                if (!fontFamilies.isEmpty()) {
                    previewFont.setFamily(fontFamilies.first());
                }
            }
        }
    }

    // Set font size for preview
    previewFont.setPointSize(12);

    // Apply font to preview label
    m_previewLabel->setFont(previewFont);

    // Update preview text
    QString previewText = "Preview: The quick brown fox jumps over the lazy dog";
    m_previewLabel->setText(previewText);
}

// Overloaded factory function for enum types with options
PropertyEditorWidget* CreatePropertyEditor(const QString& name, const QString& description,
                                         Lupine::ExportVariableType type, const QStringList& enumOptions, QWidget* parent) {
    if (type == Lupine::ExportVariableType::Enum && !enumOptions.isEmpty()) {
        return new EnumPropertyEditor(name, description, enumOptions, parent);
    }
    // Fall back to regular factory function
    return CreatePropertyEditor(name, description, type, parent);
}
