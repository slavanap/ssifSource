#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <avisynth.h>

#include <string>
using namespace std;

IScriptEnvironment *env = NULL;

PClip load(const string& fname) {
	const char* arg_names[1] = {NULL};
	AVSValue arg_vals[1] = {(LPCSTR)fname.c_str()};
	return (env->Invoke("import", AVSValue(arg_vals,1), arg_names)).AsClip();
}

void loadplugin(const string& fname) {
	const char* arg_names[1] = {NULL};
	AVSValue arg_vals[1] = {(LPCSTR)fname.c_str()};
	env->Invoke("loadplugin", AVSValue(arg_vals,1), arg_names);
}

PClip cropdetect(PClip left, PClip right, const string& outfile) {
	const char* arg_names[3] = {NULL, NULL, NULL};
	AVSValue arg_vals[3] = {left, right, (LPCSTR)outfile.c_str()};
	return (env->Invoke("CropDetect", AVSValue(arg_vals,3), arg_names)).AsClip();
}

int main(int argc, TCHAR* argv[]) {
	env = CreateScriptEnvironment();

	printf("Loading %s ...\n", argv[1]);
	string dir = (argv[2] ? argv[2] : "");
	string shortname = argv[1];
	string fleft = dir + "support_avs\\" + shortname + "_left.avs";
	string fright = dir + "support_avs\\" + shortname + "_right.avs";
	string outfile = dir + shortname + ".crop";

	DeleteFile(outfile.c_str());		// delete this file, because above AVSes use it

	PClip cleft = load(fleft);
	PClip cright = load(fright);

	loadplugin("cropdetect.dll");

	printf("Detecting crop ... \n");
	cropdetect(cleft, cright, outfile);

	printf("Completed.\n");
	return 0;
}
