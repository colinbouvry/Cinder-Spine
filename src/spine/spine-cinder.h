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

#ifndef SPINE_CINDER_H_
#define SPINE_CINDER_H_

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h" 
#include <spine/spine.h>


namespace cinder {

	class BlendMode {
		// See http://esotericsoftware.com/git/spine-runtimes/blob/spine-libgdx/spine-libgdx/src/com/esotericsoftware/spine/BlendMode.java#L37
		// for how these translate to OpenGL source/destination blend modes.
	public:
		BlendMode() {}
		BlendMode(GLenum s, GLenum d)
			: src(s)
			, dst(d) {

		}
		GLenum src;
		GLenum dst;
	};

	class RenderStates {
	public:
		RenderStates() {}
		BlendMode blendMode;
		ci::gl::TextureRef texture;
	};

	class RenderTarget {
	public:
		void draw(ci::TriMesh& mesh, RenderStates& states) {
			auto ctx = gl::context();
			ctx->enable(GL_BLEND);
			ctx->blendFunc(states.blendMode.src, states.blendMode.dst);

			// Draw the mesh we created for the attachment
			gl::ScopedGlslProg glslScope(gl::getStockShader(gl::ShaderDef().texture().color()));
			gl::ScopedTextureBind texScope(states.texture);
			gl::pushModelView();
			gl::draw(mesh);
			gl::popModelView();
		}
	};

	class Drawable {
	public:
		Drawable() {}
		virtual ~Drawable() {}
		virtual void update(float deltaTime) = 0;
		virtual void draw() {
			draw(renderTarget, states);
		}
		virtual void draw(cinder::RenderTarget& target, cinder::RenderStates& states) = 0;
		cinder::RenderStates states;
		cinder::RenderTarget renderTarget;
	};
} /* namespace cinder */

namespace spine {

	class SkeletonDrawable : public cinder::Drawable {
	public:
		SkeletonDrawable(SkeletonData* skeletonData, AnimationStateData* stateData = 0);

		virtual ~SkeletonDrawable();

		virtual void update(float deltaTime);

		virtual void draw(cinder::RenderTarget& target, cinder::RenderStates& states);

		void setUsePremultipliedAlpha(bool usePMA) { usePremultipliedAlpha = usePMA; };

		bool getUsePremultipliedAlpha() { return usePremultipliedAlpha; };

		Skeleton* skeleton;
		AnimationState* state;
		float timeScale;

		// Mesh
		ci::TriMesh vboMesh = ci::TriMesh(
			ci::TriMesh::Format()
			.positions()
			.texCoords0()
			.colors(4)
		);

	protected:
		bool ownsAnimationStateData;
		Vector<float> worldVertices;
		Vector<unsigned short> quadIndices;
		SkeletonClipping clipper;
		bool usePremultipliedAlpha;
	};

	class CINDERTextureLoader : public TextureLoader {
	public:
		virtual void load(AtlasPage &page, const String &path);

		virtual void unload(void *texture);
	};

} /* namespace spine */
#endif /* SPINE_CINDER_H_ */
