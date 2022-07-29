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
/*
 Copyright (c) 2010, Hector Sanchez-Pajares
 Aer Studio http://www.aerstudio.com
 All rights reserved.
 
 
 This is a block for TUIO Integration for the Cinder framework (http://libcinder.org)
 
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:
 
 * Redistributions of source code must retain the above copyright notice, this list of conditions and
 the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 the following disclaimer in the documentation and/or other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 */

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Log.h"

#include <iostream>
#include <memory>

#include <spine/Debug.h>
#include <spine/Log.h>
#include <spine/spine-cinder.h>

using namespace spine;
using namespace ci;
using namespace ci::app;
using namespace std;



class BasicApp : public App {
  public:
	void setup() override;
	void draw() override;
	void update() override;
	~BasicApp();
	static void callback(AnimationState* state, EventType type, TrackEntry* entry, spine::Event* event);
	static shared_ptr<SkeletonData> readSkeletonJsonData(const String& filename, Atlas* atlas, float scale);
	static shared_ptr<SkeletonData> readSkeletonBinaryData(const char* filename, Atlas* atlas, float scale);
	static void testcase(void func(SkeletonData* skeletonData, Atlas* atlas),
		const char* jsonName, const char* binaryName, const char* atlasName,
		float scale);
	static void setupRaptor(SkeletonData* skeletonData, Atlas* atlas);
	static int testcases();
	static void test(SkeletonData* skeletonData, Atlas* atlas);
	private:
		static SkeletonDrawable* mDrawable;
		DebugExtension* dbgExtension;
};

SkeletonDrawable* BasicApp::mDrawable = nullptr;

void BasicApp::setup()
{
		dbgExtension = new DebugExtension(SpineExtension::getInstance());

		SpineExtension::setInstance(dbgExtension);

		testcase(BasicApp::setupRaptor, ci::app::getAssetPath("raptor-pro.json").string().c_str(), ci::app::getAssetPath("raptor-pro.skel").string().c_str(), ci::app::getAssetPath("raptor-pma.atlas").string().c_str(), 0.5f);
		//testcase(BasicApp::setupRaptor, ci::app::getAssetPath("goblins-pro.json").string().c_str(), ci::app::getAssetPath("goblins-pro.skel").string().c_str(), ci::app::getAssetPath("goblins-pma.atlas").string().c_str(), 1.4f);
}

BasicApp::~BasicApp()
{
	dbgExtension->reportLeaks();
	if (mDrawable)
		delete mDrawable;
}

void BasicApp::draw()
{
	gl::clear( ci::Color( 0, 0, 0 ) );
	gl::setMatricesWindow( getWindowSize() );
	
	gl::ScopedColor color(ci::Color::white());
	if(mDrawable)
		mDrawable->draw();
}

void BasicApp::update()
{
	if(mDrawable)
		mDrawable->update(1/60.f);
}

template<typename T, typename... Args>
unique_ptr<T> make_unique_test(Args &&...args) {
	return unique_ptr<T>(new T(forward<Args>(args)...));
}

void BasicApp::callback(AnimationState *state, EventType type, TrackEntry *entry, spine::Event *event) {
	SP_UNUSED(state);
	const String &animationName = (entry && entry->getAnimation()) ? entry->getAnimation()->getName() : String("");

	switch (type) {
		case EventType_Start:
			printf("%d start: %s\n", entry->getTrackIndex(), animationName.buffer());
			break;
		case EventType_Interrupt:
			printf("%d interrupt: %s\n", entry->getTrackIndex(), animationName.buffer());
			break;
		case EventType_End:
			printf("%d end: %s\n", entry->getTrackIndex(), animationName.buffer());
			break;
		case EventType_Complete:
			printf("%d complete: %s\n", entry->getTrackIndex(), animationName.buffer());
			break;
		case EventType_Dispose:
			printf("%d dispose: %s\n", entry->getTrackIndex(), animationName.buffer());
			break;
		case EventType_Event:
			printf("%d event: %s, %s: %d, %f, %s %f %f\n", entry->getTrackIndex(), animationName.buffer(), event->getData().getName().buffer(), event->getIntValue(), event->getFloatValue(),
				   event->getStringValue().buffer(), event->getVolume(), event->getBalance());
			break;
	}
	fflush(stdout);
}

shared_ptr<SkeletonData> BasicApp::readSkeletonJsonData(const String &filename, Atlas *atlas, float scale) {
	SkeletonJson json(atlas);
	json.setScale(scale);
	auto skeletonData = json.readSkeletonDataFile(filename);
	if (!skeletonData) {
		printf("%s\n", json.getError().buffer());
		std::exit(0);
	}
	return shared_ptr<SkeletonData>(skeletonData);
}

shared_ptr<SkeletonData> BasicApp::readSkeletonBinaryData(const char *filename, Atlas *atlas, float scale) {
	SkeletonBinary binary(atlas);
	binary.setScale(scale);
	auto skeletonData = binary.readSkeletonDataFile(filename);
	if (!skeletonData) {
		printf("%s\n", binary.getError().buffer());
		std::exit(0);
	}
	return shared_ptr<SkeletonData>(skeletonData);
}

void BasicApp::testcase(void func(SkeletonData *skeletonData, Atlas *atlas),
			  const char *jsonName, const char *binaryName, const char *atlasName,
			  float scale) {
	SP_UNUSED(jsonName);
	CINDERTextureLoader textureLoader;
	auto atlas = make_unique_test<Atlas>(atlasName, &textureLoader);
	if (atlas->getPages().size() == 0) {
		printf("Failed to load atlas");
		std::exit(0);
	}

	auto skeletonData = readSkeletonJsonData(jsonName, atlas.get(), scale);
	func(skeletonData.get(), atlas.get());//
	if (!skeletonData) {
		printf("Failed to load skeleton data");
		std::exit(0);
	}

	/*
	skeletonData = readSkeletonBinaryData(binaryName, atlas.get(), scale);
	func(skeletonData.get(), atlas.get());
	if (!skeletonData) {
		printf("Failed to load skeleton data");
		std::exit(0);
	}*/
}

void BasicApp::setupRaptor(SkeletonData *skeletonData, Atlas *atlas) {
	SP_UNUSED(atlas);

	mDrawable = new SkeletonDrawable(skeletonData);
	mDrawable->timeScale = 1;
	mDrawable->setUsePremultipliedAlpha(true);

	Skeleton *skeleton = mDrawable->skeleton;
	skeleton->setPosition(320, 590);
	skeleton->updateWorldTransform();

	mDrawable->state->setAnimation(0, "walk", true);
	//mDrawable->state->addAnimation(1, "gun-grab", false, 2);
}


/**
 * Used for debugging purposes during runtime development
 */
void BasicApp::test(SkeletonData *skeletonData, Atlas *atlas) {
	SP_UNUSED(atlas);

	Skeleton skeleton(skeletonData);
	AnimationStateData animationStateData(skeletonData);
	AnimationState animationState(&animationStateData);
	animationState.setAnimation(0, "idle", true);

	float d = 3;
	for (int i = 0; i < 1; i++) {
		animationState.update(d);
		animationState.apply(skeleton);
		skeleton.updateWorldTransform();
		d += 0.1f;
	}
}




CINDER_APP( BasicApp, RendererGl )