#include "lupine/editor/TileBuilder.h"
#include <QImage>
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QColor>
#include <QDir>
#include <iostream>

namespace Lupine {

bool TileTextureTemplateGenerator::GenerateTemplate(const GeneratedMeshData& mesh_data, 
                                                   const glm::ivec2& template_size,
                                                   const std::string& output_path) {
    // Create a blank template image
    QImage template_image(template_size.x, template_size.y, QImage::Format_RGBA8888);
    template_image.fill(QColor(240, 240, 240, 255)); // Light gray background
    
    QPainter painter(&template_image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    
    // Draw UV grid
    DrawUVGrid(template_image.bits(), template_size.x, template_size.y, 4);
    
    // Draw face labels
    DrawFaceLabels(template_image.bits(), template_size.x, template_size.y, mesh_data);
    
    // Draw face boundaries
    painter.setPen(QPen(QColor(100, 100, 100), 2));
    
    // For each face, draw its UV bounds
    for (const auto& face_pair : mesh_data.face_uv_bounds) {
        const glm::vec4& bounds = face_pair.second;
        
        int x = static_cast<int>(bounds.x * template_size.x);
        int y = static_cast<int>(bounds.y * template_size.y);
        int width = static_cast<int>((bounds.z - bounds.x) * template_size.x);
        int height = static_cast<int>((bounds.w - bounds.y) * template_size.y);
        
        painter.drawRect(x, y, width, height);
        
        // Draw face name
        QString face_name = QString::fromStdString(TilePrimitiveMeshGenerator::GetFaceName(face_pair.first));
        QFont font("Arial", 12, QFont::Bold);
        painter.setFont(font);
        painter.setPen(QColor(50, 50, 50));
        
        QFontMetrics metrics(font);
        QRect text_rect = metrics.boundingRect(face_name);
        
        int text_x = x + (width - text_rect.width()) / 2;
        int text_y = y + (height + text_rect.height()) / 2;
        
        painter.drawText(text_x, text_y, face_name);
    }
    
    painter.end();
    
    // Save the template
    QString qpath = QString::fromStdString(output_path);
    QDir().mkpath(QFileInfo(qpath).absolutePath());
    
    if (!template_image.save(qpath)) {
        std::cerr << "Failed to save texture template: " << output_path << std::endl;
        return false;
    }
    
    std::cout << "Generated texture template: " << output_path << std::endl;
    return true;
}

bool TileTextureTemplateGenerator::GenerateFaceTemplate(MeshFace face,
                                                       const glm::vec4& face_bounds,
                                                       const glm::ivec2& template_size,
                                                       const std::string& output_path) {
    // Create a template for a specific face
    QImage face_template(template_size.x, template_size.y, QImage::Format_RGBA8888);
    face_template.fill(QColor(255, 255, 255, 255)); // White background
    
    QPainter painter(&face_template);
    painter.setRenderHint(QPainter::Antialiasing, true);
    
    // Draw a subtle grid
    painter.setPen(QPen(QColor(200, 200, 200), 1));
    int grid_size = 32;
    
    for (int x = 0; x < template_size.x; x += grid_size) {
        painter.drawLine(x, 0, x, template_size.y);
    }
    for (int y = 0; y < template_size.y; y += grid_size) {
        painter.drawLine(0, y, template_size.x, y);
    }
    
    // Draw border
    painter.setPen(QPen(QColor(100, 100, 100), 3));
    painter.drawRect(0, 0, template_size.x, template_size.y);
    
    // Draw face name in center
    QString face_name = QString::fromStdString(TilePrimitiveMeshGenerator::GetFaceName(face));
    QFont font("Arial", 24, QFont::Bold);
    painter.setFont(font);
    painter.setPen(QColor(150, 150, 150));
    
    QFontMetrics metrics(font);
    QRect text_rect = metrics.boundingRect(face_name);
    
    int text_x = (template_size.x - text_rect.width()) / 2;
    int text_y = (template_size.y + text_rect.height()) / 2;
    
    painter.drawText(text_x, text_y, face_name);
    
    painter.end();
    
    // Save the face template
    QString qpath = QString::fromStdString(output_path);
    QDir().mkpath(QFileInfo(qpath).absolutePath());
    
    if (!face_template.save(qpath)) {
        std::cerr << "Failed to save face template: " << output_path << std::endl;
        return false;
    }
    
    std::cout << "Generated face template for " << TilePrimitiveMeshGenerator::GetFaceName(face) 
              << ": " << output_path << std::endl;
    return true;
}

void TileTextureTemplateGenerator::DrawUVGrid(unsigned char* image_data, int width, int height, int channels) {
    // Draw a subtle UV grid directly on the image data
    int grid_size = 16;
    
    for (int y = 0; y < height; y += grid_size) {
        for (int x = 0; x < width; ++x) {
            int index = (y * width + x) * channels;
            if (index + 2 < width * height * channels) {
                image_data[index] = 220;     // R
                image_data[index + 1] = 220; // G
                image_data[index + 2] = 220; // B
                if (channels == 4) {
                    image_data[index + 3] = 255; // A
                }
            }
        }
    }
    
    for (int x = 0; x < width; x += grid_size) {
        for (int y = 0; y < height; ++y) {
            int index = (y * width + x) * channels;
            if (index + 2 < width * height * channels) {
                image_data[index] = 220;     // R
                image_data[index + 1] = 220; // G
                image_data[index + 2] = 220; // B
                if (channels == 4) {
                    image_data[index + 3] = 255; // A
                }
            }
        }
    }
}

void TileTextureTemplateGenerator::DrawFaceLabels(unsigned char* image_data, int width, int height, 
                                                 const GeneratedMeshData& mesh_data) {
    // This is a simplified version - in a full implementation, 
    // we would render text directly to the image data
    // For now, we'll use the QPainter approach in GenerateTemplate
}

} // namespace Lupine
