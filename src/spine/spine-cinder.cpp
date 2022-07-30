/******************************************************************************
 * Spine Runtimes License Agreement
 * Last updated September 24, 2021. Replaces all prior versions.
 *
 * Copyright (c) 2013-2021, Esoteric Software LLC
 *
 * Integration of the Spine Runtimes into software or otherwise creating
 * derivative works of the Spine Runtimes is permitted under the terms and
 * conditions of Section 2 of the Spine Editor License Agreement:
 * http://esotericsoftware.com/spine-editor-license
 *
 * Otherwise, it is permitted to integrate the Spine Runtimes into software
 * or otherwise create derivative works of the Spine Runtimes (collectively,
 * "Products"), provided that each user of the Products must obtain their own
 * Spine Editor license and redistribution of the Products in any form must
 * include this license and copyright notice.
 *
 * THE SPINE RUNTIMES ARE PROVIDED BY ESOTERIC SOFTWARE LLC "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL ESOTERIC SOFTWARE LLC BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES,
 * BUSINESS INTERRUPTION, OR LOSS OF USE, DATA, OR PROFITS) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THE SPINE RUNTIMES, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *****************************************************************************/

#include <spine/spine-cinder.h>

#include <cinder/Log.h>

#ifndef SPINE_MESH_VERTEX_COUNT_MAX
#define SPINE_MESH_VERTEX_COUNT_MAX 1000
#endif

using namespace ci;
using namespace ci::app;
using namespace std;


namespace spine {

    SkeletonDrawable::SkeletonDrawable(SkeletonData* skeletonData, AnimationStateData* stateData)
        : timeScale(1)
        , worldVertices()
        , clipper()
        , state(nullptr)
        , skeleton(nullptr)
        , usePremultipliedAlpha(false)
    {
        Bone::setYDown(true);
        worldVertices.ensureCapacity(SPINE_MESH_VERTEX_COUNT_MAX);
        skeleton = new (__FILE__, __LINE__) Skeleton(skeletonData);

        ownsAnimationStateData = stateData == 0;
        if (ownsAnimationStateData) stateData = new (__FILE__, __LINE__) AnimationStateData(skeletonData);

        state = new (__FILE__, __LINE__) AnimationState(stateData);

        quadIndices.add(0);
        quadIndices.add(1);
        quadIndices.add(2);
        quadIndices.add(2);
        quadIndices.add(3);
        quadIndices.add(0);
    }

    SkeletonDrawable::~SkeletonDrawable() {
        // vertexArray takes care of itself (just like worldVertices)
        if (ownsAnimationStateData) delete state->getData();
        delete state;
        delete skeleton;
    }

    void SkeletonDrawable::update(float deltaTime) {
        if (state && skeleton) {
            state->update(deltaTime * timeScale);
            state->apply(*skeleton);
            skeleton->updateWorldTransform();
        }
    }

    void SkeletonDrawable::draw(){
        // For each slot in the draw order array of the skeleton
        for (size_t i = 0, n = skeleton->getSlots().size(); i < n; ++i) {
            Slot* slot = skeleton->getDrawOrder()[i];

            // Fetch the currently active attachment, continue
            // with the next slot in the draw order if no
            // attachment is active on the slot
            Attachment* attachment = slot->getAttachment();
            if (!attachment) continue;

            // Fetch the blend mode from the slot and
            // translate it to the engine blend mode
            switch (slot->getData().getBlendMode()) {
            case BlendMode_Normal:
                gl::enableAlphaBlending();
                break;
            case BlendMode_Additive:
                gl::enableAdditiveBlending();
                break;
            case BlendMode_Multiply:
                break;
            case BlendMode_Screen:
                break;
            default:
                // unknown Spine blend mode, fall back to
                // normal blend mode
                gl::enableAlphaBlending();
            }

            // Calculate the tinting color based on the skeleton's color
            // and the slot's color. Each color channel is given in the
            // range [0-1], you may have to multiply by 255 and cast to
            // and int if your engine uses integer ranges for color channels.
            Color skeletonColor = skeleton->getColor();
            Color slotColor = slot->getColor();
            Color tint(skeletonColor.r * slotColor.r, skeletonColor.g * slotColor.g, skeletonColor.b * slotColor.b, skeletonColor.a * slotColor.a);

            // Fill the vertices array, indices, and texture depending on the type of attachment
            gl::TextureRef texture = nullptr;
            Vector<float>* vertices = &worldVertices;
            int verticesCount = 0;
            Vector<float>* uvs = NULL;
            Vector<unsigned short>* indices = NULL;
            int indicesCount = 0;
            Color* attachmentColor;

            if (attachment->getRTTI().isExactly(RegionAttachment::rtti)) {
                // Cast to an spRegionAttachment so we can get the rendererObject
                // and compute the world vertices
                RegionAttachment* regionAttachment = (RegionAttachment*)attachment;

                // Our engine specific Texture is stored in the AtlasRegion which was
                // assigned to the attachment on load. It represents the texture atlas
                // page that contains the image the region attachment is mapped to.
                texture = *reinterpret_cast<gl::TextureRef*>(((AtlasRegion*)regionAttachment->getRendererObject())->page->getRendererObject());

                // Ensure there is enough room for vertices
                worldVertices.setSize(8, 0);

                // Computed the world vertices positions for the 4 vertices that make up
                // the rectangular region attachment. This assumes the world transform of the
                // bone to which the slot (and hence attachment) is attached has been calculated
                // before rendering via Skeleton::updateWorldTransform(). The vertex positions
                // will be written directoy into the vertices array, with a stride of sizeof(Vertex)

                regionAttachment->computeWorldVertices(*slot, worldVertices, 0, 2);

                verticesCount = 4;
                uvs = &regionAttachment->getUVs();
                indices = &quadIndices;
                indicesCount = 6;
            }
            else if (attachment->getRTTI().isExactly(MeshAttachment::rtti)) {
                // Cast to an MeshAttachment so we can get the rendererObject
                // and compute the world vertices
                MeshAttachment* mesh = (MeshAttachment*)attachment;

                // Ensure there is enough room for vertices
                worldVertices.setSize(mesh->getWorldVerticesLength(), 0);

                // Our engine specific Texture is stored in the AtlasRegion which was
                // assigned to the attachment on load. It represents the texture atlas
                // page that contains the image the region attachment is mapped to.
                texture = *reinterpret_cast<gl::TextureRef*>(((AtlasRegion*)mesh->getRendererObject())->page->getRendererObject());

                // Computed the world vertices positions for the vertices that make up
                // the mesh attachment. This assumes the world transform of the
                // bone to which the slot (and hence attachment) is attached has been calculated
                // before rendering via Skeleton::updateWorldTransform(). The vertex positions will
                // be written directly into the vertices array, with a stride of sizeof(Vertex)
                size_t numVertices = mesh->getWorldVerticesLength() / 2;
                mesh->computeWorldVertices(*slot, 0, mesh->getWorldVerticesLength(), worldVertices.buffer(), 0, 2);

                verticesCount = mesh->getWorldVerticesLength() >> 1;
                uvs = &mesh->getUVs();
                indices = &mesh->getTriangles();
                indicesCount = mesh->getTriangles().size();
            }

            if (clipper.isClipping()) {
                clipper.clipTriangles(worldVertices, *indices, *uvs, 2);
                vertices = &clipper.getClippedVertices();
                verticesCount = clipper.getClippedVertices().size() >> 1;
                uvs = &clipper.getClippedUVs();
                indices = &clipper.getClippedTriangles();
                indicesCount = clipper.getClippedTriangles().size();
            }

            glm::ivec2 size = texture->getSize();

            auto vboMesh = TriMesh(
                TriMesh::Format()
                .positions()
                .texCoords0()
                .colors(3)
            );
            for (uint32_t ii = 0; ii < indicesCount; ++ii) {
                uint32_t index = (*indices)[ii] << 1;
                vboMesh.appendPosition(vec3((*vertices)[index], (*vertices)[index + 1], 0.f));
                vboMesh.appendTexCoord(vec2((*uvs)[index], 1.f - (*uvs)[index + 1]));
                vboMesh.appendColorRgb(ci::Color::white());
            }

            clipper.clipEnd();

            // Draw the mesh we created for the attachment
            gl::ScopedGlslProg glslScope(gl::getStockShader(gl::ShaderDef().texture()));
            gl::ScopedTextureBind texScope(texture);
            gl::pushModelView();
            gl::draw(vboMesh);
            gl::popModelView();
        }
    }
	void CINDERTextureLoader::load(AtlasPage &page, const String &path) {

		gl::TextureRef* texture;
		try {
			texture = new gl::TextureRef(gl::Texture::create(loadImage(loadFile(path.buffer()))));
		}
		catch (Exception& exc) {
			CI_LOG_EXCEPTION("failed to load image: " << path.buffer(), exc);
			return;
		}

		//if (page.magFilter == TextureFilter_Linear) texture->setSmooth(true);
		//if (page.uWrap == TextureWrap_Repeat && page.vWrap == TextureWrap_Repeat) texture->setRepeated(true);

		page.setRendererObject(texture);
		glm::ivec2 size = (*texture)->getSize();
		page.width = size.x;
		page.height = size.y;
	}

	void CINDERTextureLoader::unload(void *texture) {
		delete (gl::TextureRef*) texture;
	}

	SpineExtension *getDefaultExtension() {
		return new DefaultSpineExtension();
	}
}// namespace spine
