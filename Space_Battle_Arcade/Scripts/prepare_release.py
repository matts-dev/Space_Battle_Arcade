import sys
import os
import shutil

copy_exclusions = {
	".cmake"
	, ".ninja_deps"
	, ".ninja_log"
	, "build.ninja"
	, "CMakeCache.txt"
	, "CMakeFiles"
	, "cmake_install.cmake"
	, "config.h"
	, "hrtf_default.h"
	, "imgui.ini"
	, "openal.pc"
	, "OpenAL32.dll.manifest"
	, "SpaceBattleArcade.exe.manifest"
	, "Testing"
	, "version.h"
	, "VSInheritEnvironments.txt"
	, "_deps"
}
copy_excluded_filetypes = {
	"obj"
	, "PACKAGED_GAME"
}

if len(sys.argv) >= 2:
	release_path = sys.argv[1]
	if os.path.exists(release_path):
		dest_dir = release_path + "/PACKAGED_GAME/"
		os.makedirs(dest_dir, exist_ok=True)

		for file in os.scandir(release_path):
			#print("entry in directory", file.name) #debug print
			file_excluded = file.name in copy_exclusions
			filetype_excluded = False
			tokens = file.name.split(".")
			if len(tokens) > 0:
				potential_filetype = tokens[len(tokens) - 1]
				filetype_excluded = potential_filetype in copy_excluded_filetypes
			if not file_excluded and not filetype_excluded:
				dest_file_path = release_path + "/PACKAGED_GAME/" + file.name
				print("copying: ", file.path, " to ", dest_file_path)
				if file.is_dir():
					shutil.copytree(file.path, dest_file_path, dirs_exist_ok=True)
				else:	
					shutil.copy(file.path, dest_file_path)

else:
	print("please provide the path to the built release folder")
