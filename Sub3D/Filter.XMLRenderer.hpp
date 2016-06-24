// 3D subtitle rendering tool
// Copyright 2016 Vyacheslav Napadovsky.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once
#include <Tools.AviSynth.hpp>
#include <tinyxml2/tinyxml2.h>

namespace Filter {

	class XMLRenderer : public GenericVideoFilter {
	public:
		XMLRenderer(IScriptEnvironment* env, PClip source, const std::string& xmlfile);

		void setForcedOnly(bool value) { m_forcedOnly = value; }
		void setRecalcAllValues(bool value) { m_recalcAllValues = value; }

		void CalculateDepth();
		PVideoFrame WINAPI GetFrame(int n, IScriptEnvironment* env) override;

		static AvsParams CalculateParams;
		static AVSValue __cdecl Calculate(AVSValue args, void* user_data, IScriptEnvironment* env);
		
		static AvsParams RenderParams;
		static AVSValue __cdecl Render(AVSValue args, void* user_data, IScriptEnvironment* env);

	private:
		enum {
			DEPTH_UNDEF = MININT,
		};

		struct xGraphic {
			int width, height, x, y, depth;
			std::string filename;
			tinyxml2::XMLElement *xml_elem;
		};

		struct xEvent {
			int in_frame, out_frame;
			std::vector<xGraphic> graphics;
			tinyxml2::XMLElement *xml_elem;
			bool forced;
		};

		IScriptEnvironment *env;
		bool m_forcedOnly;
		bool m_recalcAllValues;
		std::string m_xmlfile;
		tinyxml2::XMLDocument xml;

		std::map<int, xEvent> subtitles;
		std::string graphics_path;
		std::map<int, xEvent>::const_iterator loaded_image_it;
		
		PClip overlayed_image;
		int time2framenum(const char* time);
		int str2number(tinyxml2::XMLElement* elem, const char* attr);

	};

}
