//
// Created by dev on 10/4/22.
//

#include "../model/inc/pool.h"
#include "scene.h"
#include "scene_loader.h"
void load_rungholt()
{
	auto mp = ModelPoolCreator().get();
	mp->load_all_from_obj_file("obj/rungholt.obj");
	mp->assign_single_texture_to_obj_file("obj/rungholt.obj", "obj/rungholt.tga");

	auto scene = SceneSingleton().get();
	scene->clear();

	Camera camera;
	camera.position = glm::vec3{0, 100, 0};
	camera.yaw = -90;
	camera.pitch = 0;

	for (const auto ref: mp->get_all()) {
		scene->add_object(SceneObject{glm::vec3{0, -100, -50}, {}, ref});
	}

	scene->set_camera(camera);
	scene->sort_models();
}

void load_test_single_head()
{
	auto mp = ModelPoolCreator().get();
	mp->load_all_from_obj_file("obj/african_head.obj");
	mp->assign_single_texture_to_obj_file("obj/african_head.obj", "obj/african_head_diffuse.tga");

	auto scene = SceneSingleton().get();

	Camera camera;
	camera.position = glm::vec3{-1.5, -1.2, 0.01};
	camera.yaw = -83;
	camera.pitch = 3;
	camera.fov = 25;

	scene->set_light_dir(glm::vec3{0, 0, 1});

	auto mut = mp->get_mut("obj/african_head.obj:head");
	mut->shader = RegisteredShaders::Default;

	const auto ref =  mp->get("obj/african_head.obj:head");
	scene->add_object(SceneObject{glm::vec3{0, 0, -5}, {}, ref});


	scene->set_camera(camera);
	scene->sort_models();
}

void load_test_many()
{
	auto mp = ModelPoolCreator().get();
	mp->load_all_from_obj_file("obj/african_head.obj");
	mp->assign_single_texture_to_obj_file("obj/african_head.obj", "obj/african_head_diffuse.tga");

	auto scene = SceneSingleton().get();
	scene->clear();

	Camera camera;
	camera.position = glm::vec3{0, -1, 12};
	camera.yaw = -90;
	camera.pitch = 0;

	scene->set_light_dir(glm::vec3{0, 0, 1});
	auto mut = mp->get_mut("obj/african_head.obj:head");
	mut->shader = RegisteredShaders::Default;
	auto ref = mp->get("obj/african_head.obj:head");
	scene->add_object(SceneObject{glm::vec3{0, 0, -5}, {}, ref});
	int n = 100;
	int m = 100;
	for (int i = 0; i < n; i++)
		for (int j = 0; j < m; j++)
			scene->add_object(SceneObject{glm::vec3{float(i - n / 2) * 1.5f, float(j / 2), -10.0f * j - 10}, {}, ref});


	scene->set_camera(camera);
	scene->sort_models();
}



void load_scene_diablo()
{
	auto mp = ModelPoolCreator().get();
	mp->load_all_from_obj_file("obj/diablo_pose/diablo_pose.obj");
	mp->assign_single_texture_to_obj_file("obj/diablo_pose/diablo_pose.obj", "obj/diablo_pose/diablo_pose_diffuse.tga");
	auto scene = SceneSingleton().get();
	scene->clear();

	Camera camera;
	camera.position = glm::vec3{-1.5, -1, 2};
	camera.yaw = -90;
	camera.pitch = 0;

	scene->set_light_dir(glm::vec3{0, 0, 1});

	auto mut = mp->get_mut("obj/diablo_pose/diablo_pose.obj:objDiablo3");
	mut->shader = RegisteredShaders::Default;

	auto ref = mp->get("obj/diablo_pose/diablo_pose.obj:objDiablo3");
	scene->add_object(SceneObject{glm::vec3{0, 0, -5}, {}, ref});

	scene->set_camera(camera);
	scene->sort_models();
}


void load_scene_water()
{
	auto mp = ModelPoolCreator().get();
	mp->load_all_from_obj_file("obj/water/water.obj");
	mp->assign_single_texture_to_obj_file("obj/water/water.obj", "obj/water/water.jpg");
	auto scene = SceneSingleton().get();
	scene->clear();

	Camera camera;
	camera.position = glm::vec3{2, 6, 30};
	camera.yaw = -100;
	camera.pitch = -25;
	camera.fov = 60;

	scene->set_light_dir(glm::vec3{0, 1, 0});

	auto mut = mp->get_mut("obj/water/water.obj:Plane");
	mut->shader = RegisteredShaders::Water;

	auto ref = mp->get("obj/water/water.obj:Plane");

	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			scene->add_object(SceneObject{glm::vec3{float(i) * 2.0f, 0, float(j) * 2.0f}, {}, ref});
		}
	}

	scene->set_camera(camera);
	scene->sort_models();
	//scene->allow_culling = false;
}


void load_scene_spinning_rat()
{
	auto mp = ModelPoolCreator().get();
	mp->load_all_from_obj_file("obj/spinning_rat/rattri.obj");
	mp->assign_single_texture_to_obj_file("obj/spinning_rat/rattri.obj", "obj/spinning_rat/textures/rat_low_lambert1_BaseMap.png");
	auto scene = SceneSingleton().get();
	scene->clear();

	Camera camera;
	camera.position = glm::vec3{-3, -1, 6};
	camera.yaw = -90;
	camera.pitch = -0;
	camera.fov = 80;

	scene->set_light_dir(glm::vec3{0, 0, 1});

	auto objname = "obj/spinning_rat/rattri.obj:rat1_pCylinder24";

	auto mut = mp->get_mut(objname);
	mut->shader = RegisteredShaders::Default;

	auto ref = mp->get(objname);
	scene->add_object(SceneObject{glm::vec3{0, 0, 0}, {}, ref});

	scene->set_on_update([](Scene&scene)
	{
		auto &obj = scene.get_model(0);
		obj.rotation.y += 0.1;
	});

	scene->set_camera(camera);
	scene->sort_models();
	//scene->allow_culling = false;
}


void load_sponza()
{
	auto mp = ModelPoolCreator().get();

	mp->load_all_from_obj_file("obj/sponza/models/sponza.obj", "obj/sponza/models/", "./obj/sponza/");

	auto scene = SceneSingleton().get();
	scene->clear();

	auto names = mp->get_all_names();

	Camera camera;
	camera.position = glm::vec3{-3, -1, 6};
	camera.yaw = -90;
	camera.pitch = -0;
	camera.fov = 80;

	scene->set_light_dir(glm::vec3{0, 0, 1});
	for (const auto ref: mp->get_all()) {
		scene->add_object(SceneObject{glm::vec3{0, -100, -50}, {}, ref});
	}

	scene->set_camera(camera);
	scene->sort_models();
	//scene->allow_culling = false;
}

void register_predefined_scenes()
{
	auto scene_loader = SceneLoaderSingleton().get();
	scene_loader->register_load_scene("test_single_head", load_test_single_head);
	scene_loader->register_load_scene("test_many_heads", load_test_many);
	scene_loader->register_load_scene("diablo", load_scene_diablo);
	scene_loader->register_load_scene("water", load_scene_water);
	scene_loader->register_load_scene("spinning_rat", load_scene_spinning_rat);
	scene_loader->register_load_scene("default", load_test_single_head);
}
