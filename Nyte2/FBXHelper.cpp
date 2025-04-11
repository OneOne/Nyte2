#include "FBXHelper.h"
#include "FileHelper.h"
#include <unordered_map>
#include <fstream>

void FBXHelper::loadFBX(FBXScene& _fbx)
{
    std::vector<octet> rawData = FileHelper::readFile(_fbx.filePath);

    // Ignoring certain nodes will only stop them from being processed not tokenised (i.e. they will still be in the tree)
    ofbx::LoadFlags flags =
        //		ofbx::LoadFlags::IGNORE_MODELS |
        ofbx::LoadFlags::IGNORE_BLEND_SHAPES |
        ofbx::LoadFlags::IGNORE_CAMERAS |
        ofbx::LoadFlags::IGNORE_LIGHTS |
        //		ofbx::LoadFlags::IGNORE_TEXTURES |
        ofbx::LoadFlags::IGNORE_SKIN |
        ofbx::LoadFlags::IGNORE_BONES |
        ofbx::LoadFlags::IGNORE_PIVOTS |
        //		ofbx::LoadFlags::IGNORE_MATERIALS |
        ofbx::LoadFlags::IGNORE_POSES |
        ofbx::LoadFlags::IGNORE_VIDEOS |
        ofbx::LoadFlags::IGNORE_LIMBS |
        //		ofbx::LoadFlags::IGNORE_MESHES |
        ofbx::LoadFlags::IGNORE_ANIMATIONS;

    ofbx::IScene* scene = ofbx::load((ofbx::u8*)rawData.data(), (ofbx::usize)rawData.size(), (ofbx::u16)flags);

    int meshCount = scene->getMeshCount();
    std::unordered_map<ofbx::u64, int> allMaterialIndices;

    for (int i = 0; i < meshCount; ++i)
    {
        const ofbx::Mesh* mesh = scene->getMesh(i);
        const ofbx::GeometryData& geometryData = mesh->getGeometryData();

        ofbx::Vec3Attributes positions = geometryData.getPositions();
        ofbx::Vec3Attributes normals = geometryData.getNormals();
        //ofbx::Vec3Attributes tangents = geometryData.getTangents();
        ofbx::Vec2Attributes uv0 = geometryData.getUVs(0);

	// TODO: improve import speed by reworking VB data (multi-buffer, stride)
        static bool GOTTA_GO_FAST = true; // hack to speed up the importer

        FBXMesh fbxMesh;
        int vertexCount = positions.count;
        for (int idx = 0; idx < vertexCount; ++idx)
        {
            FBXVertex vertex;
            vertex.position = positions.get(idx);
            vertex.normal = normals.get(idx);
            //vertex.tangent = tangents.get(idx);
            vertex.uv = uv0.get(idx);

            if (GOTTA_GO_FAST)
            {
                fbxMesh.m_vertices.emplace_back(
                    positions.get(idx),
                    normals.get(idx),
                    //tangents.get(idx),
                    uv0.get(idx)
                );
                u32 index = (u32)fbxMesh.m_vertices.size() - 1;
                fbxMesh.m_indices.push_back(index);
            }
            else
            {
                auto foundIt = std::find(fbxMesh.m_vertices.rbegin(), fbxMesh.m_vertices.rend(), vertex);
                if (foundIt == fbxMesh.m_vertices.rend())
                {
                    fbxMesh.m_vertices.emplace_back(
                        positions.get(idx),
                        normals.get(idx),
                        //tangents.get(idx),
                        uv0.get(idx)
                    );
                    u32 index = (u32)fbxMesh.m_vertices.size() - 1;
                    fbxMesh.m_indices.push_back(index);
                }
                else
                {
                    // vertex already registered
                    u32 index = (u32)std::distance(fbxMesh.m_vertices.begin(), foundIt.base()) - 1;
                    fbxMesh.m_indices.push_back(index);
                }
            }
        }
        

        //if (mesh->getMaterialCount() > 0)
        //{
        //    const ofbx::Material* material = mesh->getMaterial(0);
        //
        //    auto it = allMaterialIndices.find(material->id);
        //    if (it == allMaterialIndices.end())
        //    {
        //        // Add new material
        //        FBXMaterial fbxMaterial;
        //
        //        char tmp[1024];
        //
        //        material->getTexture(ofbx::Texture::DIFFUSE)->getFileName().toString(tmp);
        //        fbxMaterial.diffuse = std::string(tmp);
        //
        //        material->getTexture(ofbx::Texture::NORMAL)->getFileName().toString(tmp);
        //        fbxMaterial.normal = std::string(tmp);
        //
        //        material->getTexture(ofbx::Texture::SPECULAR)->getFileName().toString(tmp);
        //        fbxMaterial.specular = std::string(tmp);
        //
        //        material->getTexture(ofbx::Texture::SHININESS)->getFileName().toString(tmp);
        //        fbxMaterial.shininess = std::string(tmp);
        //
        //        _fbx.materials.push_back(fbxMaterial);
        //        int materialIndex = _fbx.materials.size() - 1;
        //
        //        allMaterialIndices[material->id] = materialIndex;
        //        fbxMesh.materialIndex = materialIndex;
        //    }
        //    else
        //    {
        //        fbxMesh.materialIndex = it->second;
        //    }
        //}

        _fbx.meshes.push_back(fbxMesh);
    }

    // Cache result
    //std::string cachePath = getCachePath(_fbx);
    //std::ofstream fileStream(cachePath, std::ios::out | std::ios::binary | std::ios::app);
    //for (FBXMesh& mesh : _fbx.meshes)
    //{
    //    fileStream.write(mesh, sizeof FBXMesh);
    //}
    //fs.write(surname, sizeof surname);
    //fs.write(reinterpret_cast<const char*>(&age), sizeof age);
    //fileStream.close();
}
