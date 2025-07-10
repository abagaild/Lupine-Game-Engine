#include "lupine/editor/TileBuilder.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QFileInfo>
#include <iostream>

namespace Lupine {

bool TileOBJExporter::ExportToOBJ(const TileBuilderData& tile_data, 
                                 const std::string& output_path,
                                 bool export_materials) {
    QFile file(QString::fromStdString(output_path));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        std::cerr << "Failed to open OBJ file for writing: " << output_path << std::endl;
        return false;
    }
    
    QTextStream out(&file);
    QFileInfo file_info(QString::fromStdString(output_path));
    QString base_name = file_info.baseName();
    
    // Write header
    out << "# OBJ file exported from Lupine Tile Builder\n";
    out << "# Tile: " << QString::fromStdString(tile_data.name) << "\n";
    out << "# Vertices: " << tile_data.mesh_data.vertices.size() / 8 << "\n";
    out << "# Faces: " << tile_data.mesh_data.indices.size() / 3 << "\n\n";
    
    // Write material library reference if exporting materials
    if (export_materials && !tile_data.face_textures.empty()) {
        QString mtl_filename = base_name + ".mtl";
        out << "mtllib " << mtl_filename << "\n\n";
        
        // Write material file
        QString mtl_path = file_info.absolutePath() + "/" + mtl_filename;
        TileOBJExporter::WriteMaterialFile(tile_data, mtl_path.toStdString());
    }
    
    // Write vertices
    out << "# Vertices\n";
    for (size_t i = 0; i < tile_data.mesh_data.vertices.size(); i += 8) {
        float x = tile_data.mesh_data.vertices[i];
        float y = tile_data.mesh_data.vertices[i + 1];
        float z = tile_data.mesh_data.vertices[i + 2];
        out << "v " << x << " " << y << " " << z << "\n";
    }
    out << "\n";
    
    // Write texture coordinates
    out << "# Texture coordinates\n";
    for (size_t i = 0; i < tile_data.mesh_data.vertices.size(); i += 8) {
        float u = tile_data.mesh_data.vertices[i + 6];
        float v = tile_data.mesh_data.vertices[i + 7];
        out << "vt " << u << " " << v << "\n";
    }
    out << "\n";
    
    // Write normals
    out << "# Normals\n";
    for (size_t i = 0; i < tile_data.mesh_data.vertices.size(); i += 8) {
        float nx = tile_data.mesh_data.vertices[i + 3];
        float ny = tile_data.mesh_data.vertices[i + 4];
        float nz = tile_data.mesh_data.vertices[i + 5];
        out << "vn " << nx << " " << ny << " " << nz << "\n";
    }
    out << "\n";
    
    // Write object
    out << "# Object\n";
    out << "o " << QString::fromStdString(tile_data.name) << "\n\n";
    
    // Group faces by material if materials are exported
    if (export_materials && !tile_data.face_textures.empty()) {
        // For now, use a single material for all faces
        // In a more advanced implementation, we could group faces by texture
        out << "usemtl tile_material\n";
    }
    
    // Write faces
    out << "# Faces\n";
    for (size_t i = 0; i < tile_data.mesh_data.indices.size(); i += 3) {
        unsigned int v1 = tile_data.mesh_data.indices[i] + 1;     // OBJ is 1-indexed
        unsigned int v2 = tile_data.mesh_data.indices[i + 1] + 1;
        unsigned int v3 = tile_data.mesh_data.indices[i + 2] + 1;
        
        // Format: f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3
        out << "f " << v1 << "/" << v1 << "/" << v1 << " "
            << v2 << "/" << v2 << "/" << v2 << " "
            << v3 << "/" << v3 << "/" << v3 << "\n";
    }
    
    file.close();
    
    std::cout << "Exported OBJ file: " << output_path << std::endl;
    return true;
}

bool TileOBJExporter::WriteMaterialFile(const TileBuilderData& tile_data,
                                       const std::string& mtl_path) {
    QFile file(QString::fromStdString(mtl_path));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        std::cerr << "Failed to open MTL file for writing: " << mtl_path << std::endl;
        return false;
    }
    
    QTextStream out(&file);
    
    // Write header
    out << "# MTL file exported from Lupine Tile Builder\n";
    out << "# Material for tile: " << QString::fromStdString(tile_data.name) << "\n\n";
    
    // Write material
    out << "newmtl tile_material\n";
    out << "Ka 1.0 1.0 1.0\n";  // Ambient color
    out << "Kd 1.0 1.0 1.0\n";  // Diffuse color
    out << "Ks 0.0 0.0 0.0\n";  // Specular color
    out << "Ns 0.0\n";           // Specular exponent
    out << "d 1.0\n";            // Transparency
    out << "illum 1\n";          // Illumination model
    
    // Find a texture to use (use the first available texture)
    for (const auto& face_pair : tile_data.face_textures) {
        if (!face_pair.second.texture_path.empty()) {
            QString texture_path = QString::fromStdString(face_pair.second.texture_path);
            QFileInfo texture_info(texture_path);
            
            // Use relative path if possible
            QFileInfo mtl_info(QString::fromStdString(mtl_path));
            QString relative_path = mtl_info.dir().relativeFilePath(texture_path);
            
            out << "map_Kd " << relative_path << "\n";
            break;
        }
    }
    
    file.close();
    
    std::cout << "Exported MTL file: " << mtl_path << std::endl;
    return true;
}

std::string TileOBJExporter::GetRelativePath(const std::string& from, const std::string& to) {
    QDir from_dir(QString::fromStdString(from));
    QString relative = from_dir.relativeFilePath(QString::fromStdString(to));
    return relative.toStdString();
}

} // namespace Lupine
