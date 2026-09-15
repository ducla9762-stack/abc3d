#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

#include "sceneFile.h"
#include "mesh.h" //ensureExtension

static void WriteVec(std::ostream& out, const char* key, const Vec3f& v){
	out << key << " " << v.x << " " << v.y << " " << v.z << "\n"; 
}

bool WriteSceneFile(const std::string& filePath, const SceneFile& doc) {
	std::ofstream file(ensureExtension(filePath, ".abc3d"));
	if (!file.is_open()) return false;

	file << "ABC3D " << SCENE_FILE_VERSION << "\n";

	file << "[scene]\n";
	WriteVec(file, "camera", doc.camera); 
	file << "grid " << (doc.grid ? 1 : 0) << "\n"; 
	file << "points " << (doc.points ? 1 : 0) << "\n"; 
	file << "selection"; 
	
	for (uint id : doc.selection) 
		file << " " << id; 

	file << "\n[end]\n"; 

	for (const SceneFileObject& obj : doc.objects) {
		//weird formatting, but basically just putting the name in quotation marks
		file << "[object id=" << obj.id << " name = \"" << obj.name << "\"]\n"; 
		WriteVec(file, "pos", obj.pos); 
		WriteVec(file, "rot", obj.rot); 
		WriteVec(file, "scale", obj.scale); 

		file << "geometry obj\n"; 
		file << obj.objText; //appendObj already newline terminates every line
		file << "[end]\n"; 
	}

	file << "[specs]\n"; 
	for (const std::string& line : doc.specs)
		file << line << "\n"; 
	file << "[end]\n";

	return !file.fail(); 
}

//Helper function for reading: Pulls "id = " and "name="..." out of an [object ...] header line
static void ParseObjectHeader(const std::string& line, SceneFileObject& obj){
	const size_t idPos = line.find("id="); 
	if (idPos != std::string::npos)
		obj.id = static_cast<uint>(std::strtoul(line.c_str() + idPos + 3, nullptr, 10)); 
	
	const size_t open = line.find('"'); 
	const size_t close = (open == std::string::npos) ? std::string::npos : line.find('"', open + 1); 

	if (close != std::string::npos)
		obj.name = line.substr(open + 1, close - open - 1); 
}

bool ReadSceneFile(const std::string& filePath, SceneFile& doc) {
	std::ifstream file(filePath); 
	if (!file.is_open())
		return false; 
	
	std::string line; 
	if (!std::getline(file, line))
		return false; 
	
	//Header: "ABC3D <version>"
	std::string abc; //This is a weird name. If anyone has any suggestions to change, please say so. 
	int version = 0; 
	std::istringstream header(line); 
	header >> abc >> version; 

	if (header.fail() || abc != "ABC3D")
		return false; 
	
	//Written by newer ABC3D
	if (version > SCENE_FILE_VERSION)
		return false;

	SceneFile parsed; 
	enum Section {NONE, SCENE, OBJECT, SPECS};
	Section section = NONE; 
	SceneFileObject current; 

	while (std::getline(file, line)) {
		//For cross platform case (if they save file, open in linux, and save again)
		if (!line.empty() && line.back() == '\r')
			line.pop_back(); 
		
		if (line.empty())
			continue; 
		
		if (line == "[end]") {
			if (section == OBJECT) {
				parsed.objects.push_back(std::move(current)); 
				current = SceneFileObject(); 
			}
			section = NONE; 
			continue; 
		}

		if (line == "[scene]") {
			section = SCENE; 
			continue; 
		}

		if (line == "[specs]") {
			section = SPECS; 
			continue; 
		}
		
		if (line.rfind("[object", 0) == 0) {
			section = OBJECT; 
			ParseObjectHeader(line, current); 
			continue; 
		}

		std::istringstream in(line); 
		std::string key; 
		in >> key; 

		if (section == SCENE) {
			if (key == "camera")
				in >> parsed.camera.x >> parsed.camera.y >> parsed.camera.z; 
			
			else if (key == "grid") {
				int v = 1; 
				in >> v; 
				parsed.grid = (v != 0); 
			}

			else if (key == "points") {
				int v = 1; 
				in >> v; 
				parsed.points = (v != 0); 
			}

			else if (key == "selection") {
				uint id; 
				while (in >> id)
					parsed.selection.push_back(id); 
			}

			else {
				//Should we through an exception here? 
				continue; 
			}
		}

		else if (section == OBJECT){
			if (key == "pos")
				in >> current.pos.x >> current.pos.y >> current.pos.z;
			else if (key == "rot")
				in >> current.rot.x >> current.rot.y >> current.rot.z;
			else if (key == "scale")
				in >> current.scale.x >> current.scale.y >> current.scale.z;
			else if (key == "geometry")
				continue; //obj is the only encoding right now
			else {
				current.objText += line; 
				current.objText += "\n"; 
			}
		}

		else if (section == SPECS) {
			parsed.specs.push_back(line); 
		}
	}

	if (file.bad())
		return false; 
	
	doc = std::move(parsed); 
	return true; 
}
