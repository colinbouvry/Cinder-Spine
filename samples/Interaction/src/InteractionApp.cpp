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
  Copyright (c) 2022, Colin BOUVRY
  Aer Studio http://www.colinbouvry.com
  All rights reserved.


  This is a block for Spine Integration for the Cinder framework (http://libcinder.org)

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



class InteractionApp : public App {
public:
	void setup() override;
	void draw() override;
	void update() override;
	void keyDown(KeyEvent event) override;
	void mouseDown(MouseEvent event) override;
	~InteractionApp();
	static void callback(AnimationState* state, EventType type, TrackEntry* entry, spine::Event* event);
	static shared_ptr<SkeletonData> readSkeletonJsonData(const String& filename, Atlas* atlas, float scale);
	static shared_ptr<SkeletonData> readSkeletonBinaryData(const char* filename, Atlas* atlas, float scale);
protected:
	SkeletonDrawable* mDrawable;
	DebugExtension* dbgExtension;
	CINDERTextureLoader textureLoader;
	spine::Atlas* atlas;
	spine::SkeletonData* skeletonData;
};

void InteractionApp::setup()
{
	dbgExtension = new DebugExtension(SpineExtension::getInstance());

	SpineExtension::setInstance(dbgExtension);

	string jsonName = ci::app::getAssetPath("spinosaurus-ess.json").string();
	string atlasName = ci::app::getAssetPath("spinosaurus-pro.atlas").string();
	float scale = 0.5f;

	atlas = new (__FILE__, __LINE__) Atlas(atlasName.c_str(), &textureLoader);
	assert(atlas);

	SkeletonJson json(atlas);
	skeletonData = json.readSkeletonDataFile(jsonName.c_str());
	assert(skeletonData);

	mDrawable = new SkeletonDrawable(skeletonData);
	mDrawable->timeScale = 1;
	mDrawable->setUsePremultipliedAlpha(false);

	Skeleton* skeleton = mDrawable->skeleton;
	skeleton->setPosition(960, 540);
	skeleton->updateWorldTransform();

	TrackEntry* entry = mDrawable->state->setAnimation(0, "animation", false);

	mDrawable->update(0.f);
}

InteractionApp::~InteractionApp()
{
	dbgExtension->reportLeaks();
	if (mDrawable)
		delete mDrawable;
}

void InteractionApp::draw()
{
	gl::clear(ci::Color(0, 0, 0));
	gl::setMatricesWindow(getWindowSize());

	gl::ScopedColor color(ci::Color::white());

	gl::pushMatrices();

	if (mDrawable)
		mDrawable->Drawable::draw();
	gl::popMatrices();
}

void InteractionApp::update()
{
	if (mDrawable)
		mDrawable->update(1 / 60.f);
}


void InteractionApp::callback(AnimationState* state, EventType type, TrackEntry* entry, spine::Event* event) {
	SP_UNUSED(state);
	const String& animationName = (entry && entry->getAnimation()) ? entry->getAnimation()->getName() : String("");

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

shared_ptr<SkeletonData> InteractionApp::readSkeletonJsonData(const String& filename, Atlas* atlas, float scale) {
	SkeletonJson json(atlas);
	json.setScale(scale);
	auto skeletonData = json.readSkeletonDataFile(filename);
	if (!skeletonData) {
		printf("%s\n", json.getError().buffer());
		std::exit(0);
	}
	return shared_ptr<SkeletonData>(skeletonData);
}

shared_ptr<SkeletonData> InteractionApp::readSkeletonBinaryData(const char* filename, Atlas* atlas, float scale) {
	SkeletonBinary binary(atlas);
	binary.setScale(scale);
	auto skeletonData = binary.readSkeletonDataFile(filename);
	if (!skeletonData) {
		printf("%s\n", binary.getError().buffer());
		std::exit(0);
	}
	return shared_ptr<SkeletonData>(skeletonData);
}

void InteractionApp::keyDown(KeyEvent event)
{
	switch (event.getCode())
	{
	case KeyEvent::KEY_ESCAPE:
		// quit the application
		quit();
		return;
	case KeyEvent::KEY_f:
		setFullScreen(!isFullScreen());
		break;
	case KeyEvent::KEY_SPACE:
		mDrawable->state->setAnimation(0, "animation", false);
	break;
	}
}

void InteractionApp::mouseDown(MouseEvent event)
{
	auto hitBoundingBoxAttachment = mDrawable->bounds->containsPoint(event.getX(), event.getY());
	if (hitBoundingBoxAttachment) {
		string name = string(hitBoundingBoxAttachment->getName().buffer());
		CI_LOG_I("hit " << name);
		if (name == "play_boundingbox") {
			mDrawable->state->setAnimation(1, "play", false);
			mDrawable->state->addEmptyAnimation(1, 0.1, 0);
		}
		else if (name == "settings_boundingbox") {
			mDrawable->state->setAnimation(1, "settings", false);
			mDrawable->state->addEmptyAnimation(1, 0.1, 0);
		}
		else if (name == "quit_boundingbox") {
			mDrawable->state->setAnimation(1, "quit", false);
			mDrawable->state->addEmptyAnimation(1, 0.1, 0);
		}
	}
}

CINDER_APP(InteractionApp, RendererGl)