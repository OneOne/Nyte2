#include "FBXHelper.h"
#include "FileHelper.h"
#include <unordered_map>


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

        FBXMesh fbxMesh;
        int indexCount = positions.count;
        for (int idx = 0; idx < indexCount; ++idx)
        {
            //int index = positions.indices[idx];
            //if (index >= fbxMesh.m_vertices.size())
            {
                FBXVertex vertex;
                vertex.position = positions.get(idx);
                vertex.normal = normals.get(idx);
                //vertex.tangent = tangents.get(idx);
                vertex.uv = uv0.get(idx);

                fbxMesh.m_vertices.emplace_back(
                    positions.get(idx),
                    normals.get(idx),
                    //tangents.get(idx),
                    uv0.get(idx)
                );
            }

            fbxMesh.m_indices.push_back((u32)fbxMesh.m_indices.size());
        }
        

        //if (mesh->getMaterialCount() > 0)
        //{
        //    const ofbx::Material* material = mesh->getMaterial(0);

        //    auto it = allMaterialIndices.find(material->id);
        //    if (it == allMaterialIndices.end())
        //    {
        //        // Add new material
        //        FBXMaterial fbxMaterial;

        //        char tmp[1024];

        //        material->getTexture(ofbx::Texture::DIFFUSE)->getFileName().toString(tmp);
        //        fbxMaterial.diffuse = std::string(tmp);

        //        material->getTexture(ofbx::Texture::NORMAL)->getFileName().toString(tmp);
        //        fbxMaterial.normal = std::string(tmp);

        //        material->getTexture(ofbx::Texture::SPECULAR)->getFileName().toString(tmp);
        //        fbxMaterial.specular = std::string(tmp);

        //        material->getTexture(ofbx::Texture::SHININESS)->getFileName().toString(tmp);
        //        fbxMaterial.shininess = std::string(tmp);

        //        _fbx.materials.push_back(fbxMaterial);
        //        int materialIndex = _fbx.materials.size() - 1;

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
}