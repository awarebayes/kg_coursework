//
// Created by dev on 7/10/22.
//
#include "../inc/pool.h"

ModelRef ModelPool::get(const std::string &name) {
	if (pool.find(name) == pool.end())
		throw std::runtime_error("Model not found in pool");
	return pool.at(name)->get_ref();
}

std::shared_ptr<Model> ModelPool::get_mut(const std::string &name) {
	if (pool.find(name) == pool.end())
		throw std::runtime_error("Model not found in pool");
	return pool.at(name);
}

void ModelPool::load_all_from_obj_file(const std::string &filename, const std::string &mtl_search_path, const std::string &texture_search_path) {
	tinyobj::ObjReaderConfig reader_config;
	reader_config.mtl_search_path = mtl_search_path; // Path to material files
	tinyobj::ObjReader reader;

	if (!reader.ParseFromFile(filename, reader_config)) {
		if (!reader.Error().empty()) {
			std::cerr << "TinyObjReader: " << reader.Error();
		}
		exit(1);
	}

	for (int i = 0; i < reader.GetShapes().size(); i++) {
		auto model = std::make_shared<Model>(reader, i, texture_search_path);
		model->id = id_counter++;
		std::string model_name = filename + ":" + reader.GetShapes()[i].name;
		pool.insert(std::make_pair(model_name, model));

	}
}
void ModelPool::assign_single_texture_to_obj_file(const std::string &obj_filename, const std::string &texture_filename) {
	tinyobj::ObjReaderConfig reader_config;
	reader_config.mtl_search_path = "./";
	tinyobj::ObjReader reader;

	if (!reader.ParseFromFile(obj_filename, reader_config)) {
		if (!reader.Error().empty()) {
			std::cerr << "TinyObjReader: " << reader.Error();
		}
		exit(1);
	}

	for (int i = 0; i < reader.GetShapes().size(); i++) {
		auto model = get_mut(obj_filename + ":" + reader.GetShapes()[i].name);
		model->load_texture(texture_filename);
	}
}
std::vector<ModelRef> ModelPool::get_all() {
	std::vector<ModelRef> models;
	for (auto &model : pool) {
		if (model.second->texture)
			models.push_back(model.second->get_ref());
	}
	return models;
}
void ModelPool::clear() {
	pool.clear();
}
ModelPool::ModelPool() {
	default_texture = std::make_shared<Texture>(Texture::get_default());
}
std::vector<std::string> ModelPool::get_all_names() {
	std::vector<std::string> result;
	for (auto &model : pool) {
		result.push_back(model.first);
	}
	return result;
}
