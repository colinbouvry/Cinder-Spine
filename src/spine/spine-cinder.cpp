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

ci::BlendMode normal = ci::BlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
ci::BlendMode additive = ci::BlendMode(GL_SRC_ALPHA, GL_ONE);
ci::BlendMode multiply = ci::BlendMode(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
ci::BlendMode screen = ci::BlendMode(GL_ONE, GL_ONE_MINUS_SRC_COLOR);

ci::BlendMode normalPma = ci::BlendMode(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
ci::BlendMode additivePma = ci::BlendMode(GL_ONE, GL_ONE);
ci::BlendMode multiplyPma = ci::BlendMode(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
ci::BlendMode screenPma = ci::BlendMode(GL_ONE, GL_ONE_MINUS_SRC_COLOR);

namespace spine {

    SkeletonDrawable::SkeletonDrawable(SkeletonData* skeletonData, AnimationStateData* stateData)
        : timeScale(1)
        , worldVertices()
        , clipper()
        , state(nullptr)
        , skeleton(nullptr)
        , usePremultipliedAlpha(false)
        , Drawable()
    {
        Bone::setYDown(true);
        worldVertices.ensureCapacity(SPINE_MESH_VERTEX_COUNT_MAX);
        skeleton = new (__FILE__, __LINE__) Skeleton(skeletonData);

        bounds = new SkeletonBounds();

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
            bounds->update(*skeleton, false);
        }
    }

    void SkeletonDrawable::draw(RenderTarget& target, RenderStates& states){

        // Clear
        vboMesh.clear();
        // Texture
        gl::TextureRef texture = nullptr;
        states.texture = nullptr;

        // Early out if skeleton is invisible
        if (skeleton->getColor().a == 0) return;


        // For each slot in the draw order array of the skeleton
        for (size_t i = 0, n = skeleton->getSlots().size(); i < n; ++i) {
            Slot* slot = skeleton->getDrawOrder()[i];

            // Fetch the currently active attachment, continue
            // with the next slot in the draw order if no
            // attachment is active on the slot
            Attachment* attachment = slot->getAttachment();
            if (!attachment) continue;

            // Fill the vertices array, indices, and texture depending on the type of attachment
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
                attachmentColor = &regionAttachment->getColor();

                // Early out if the slot color is 0
                if (attachmentColor->a == 0) {
                    clipper.clipEnd(*slot);
                    continue;
                }

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
                attachmentColor = &mesh->getColor();

                // Early out if the slot color is 0
                if (attachmentColor->a == 0) {
                    clipper.clipEnd(*slot);
                    continue;
                }

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
            else if (attachment->getRTTI().isExactly(ClippingAttachment::rtti)) {
                ClippingAttachment* clip = (ClippingAttachment*)slot->getAttachment();
                clipper.clipStart(*slot, clip);
                continue;
            }
            else
                continue;

            ci::BlendMode blend;
            if (!usePremultipliedAlpha) {
                switch (slot->getData().getBlendMode()) {
                case BlendMode_Normal:
                    blend = normal;
                    break;
                case BlendMode_Additive:
                    blend = additive;
                    break;
                case BlendMode_Multiply:
                    blend = multiply;
                    break;
                case BlendMode_Screen:
                    blend = screen;
                    break;
                default:
                    blend = normal;
                }
            }
            else {
                switch (slot->getData().getBlendMode()) {
                case BlendMode_Normal:
                    blend = normalPma;
                    break;
                case BlendMode_Additive:
                    blend = additivePma;
                    break;
                case BlendMode_Multiply:
                    blend = multiplyPma;
                    break;
                case BlendMode_Screen:
                    blend = screenPma;
                    break;
                default:
                    blend = normalPma;
                }
            }

            if (states.texture == 0) states.texture = texture;

            if (states.blendMode.src != blend.src || states.blendMode.dst != blend.dst || states.texture != texture) {
                target.draw(vboMesh, states);
                vboMesh.clear();
                states.blendMode = blend;
                states.texture = texture;
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
            uint8_t r = static_cast<uint8_t>(skeleton->getColor().r * slot->getColor().r * attachmentColor->r * 255.f);
            uint8_t g = static_cast<uint8_t>(skeleton->getColor().g * slot->getColor().g * attachmentColor->g * 255.f);
            uint8_t b = static_cast<uint8_t>(skeleton->getColor().b * slot->getColor().b * attachmentColor->b * 255.f);
            uint8_t a = static_cast<uint8_t>(skeleton->getColor().a * slot->getColor().a * attachmentColor->a * 255.f);

            for (uint32_t ii = 0; ii < indicesCount; ++ii) {
                uint32_t index = (*indices)[ii] << 1;
                vboMesh.appendPosition(vec3((*vertices)[index], (*vertices)[index + 1], 0.f));
                vboMesh.appendTexCoord(vec2((*uvs)[index], 1.f - (*uvs)[index + 1]));
                vboMesh.appendColorRgba(ci::ColorA(r, g, b, a)/255.f);
            }

            clipper.clipEnd(*slot);
        }
        target.draw(vboMesh, states);

        clipper.clipEnd();
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

        if (page.magFilter == TextureFilter_Linear)
            texture->get()->setMagFilter(GL_LINEAR);

        if (page.uWrap == TextureWrap_Repeat && page.vWrap == TextureWrap_Repeat)
            texture->get()->setWrap(GL_REPEAT, GL_REPEAT);

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
