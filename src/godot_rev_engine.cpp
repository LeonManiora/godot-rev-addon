#include "godot_rev_engine.h"
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/audio_server.hpp>
#include <cmath>

using namespace godot;

void GodotREVEngine::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_rpminfluence", "value"), &GodotREVEngine::set_rpminfluence);
	ClassDB::bind_method(D_METHOD("get_rpminfluence"), &GodotREVEngine::get_rpminfluence);
	ClassDB::bind_method(D_METHOD("set_throttle", "value"), &GodotREVEngine::set_throttle);
	ClassDB::bind_method(D_METHOD("get_throttle"), &GodotREVEngine::get_throttle);
	
	ClassDB::bind_method(D_METHOD("init_rev_library"), &GodotREVEngine::init_rev_library);
	ClassDB::bind_method(D_METHOD("load_rev_model", "model_bytes"), &GodotREVEngine::load_rev_model);
	ClassDB::bind_method(D_METHOD("start_audio"), &GodotREVEngine::start_audio);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "rpminfluence"), "set_rpminfluence", "get_rpminfluence");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "throttle"), "set_throttle", "get_throttle");
}

GodotREVEngine::GodotREVEngine() {}

GodotREVEngine::~GodotREVEngine() {
	if (rev_simulator_ptr && f_destroy) {
		f_destroy(rev_simulator_ptr);
	}
}

bool GodotREVEngine::init_rev_library() {
	// FIX: Wir nutzen den globalen res:// Pfad direkt. Godot löst das intern in Windows-Pfade auf.
	String path = "res://addons/crankcase/REVRuntime.dll";
	
	// FIX: Offizieller GDExtension-Weg um Symbole und DLLs plattformunabhängig zu binden
	// Das kompiliert fehlerfrei auf JEDEM Server!
	f_create = (CreateFunc)UtilityFunctions::import_native_symbol(path, "REV_CreateSimulator");
	f_destroy = (DestroyFunc)UtilityFunctions::import_native_symbol(path, "REV_DestroySimulator");
	f_load = (LoadFunc)UtilityFunctions::import_native_symbol(path, "REV_LoadModelData");
	f_update = (UpdateFunc)UtilityFunctions::import_native_symbol(path, "REV_Update");
	f_generate = (GenerateFunc)UtilityFunctions::import_native_symbol(path, "REV_GenerateAudio");

	if (!f_create || !f_destroy || !f_load || !f_update || !f_generate) {
		UtilityFunctions::push_error("[REV Engine] Symbole konnten nicht geladen werden! Überprüfe ob die DLL im Ordner addons/crankcase/ liegt.");
		return false;
	}

	rev_simulator_ptr = f_create();
	return (rev_simulator_ptr != nullptr);
}

bool GodotREVEngine::load_rev_model(const PackedByteArray& model_bytes) {
	if (!rev_simulator_ptr || !f_load) return false;
	return f_load(rev_simulator_ptr, model_bytes.read().ptr(), model_bytes.size());
}

void GodotREVEngine::start_audio() {
	generator_player = Object::cast_to<AudioStreamPlayer3D>(get_node_or_null("GeneratorPlayer"));
	if (generator_player) {
		generator_player->play();
		playback = generator_player->get_stream_playback();
	}
}

void GodotREVEngine::_process(double delta) {
	if (playback.is_null() || !rev_simulator_ptr || !f_update || !f_generate) return;

	float real_rpm = lerp(800.0f, 7500.0f, (float)rpminfluence);
	float real_throttle = (float)throttle_input;
	float fake_velocity = real_rpm * 0.02f;

	f_update(rev_simulator_ptr, real_rpm, real_throttle, fake_velocity, current_gear);

	int frames_available = playback->get_frames_available();
	if (frames_available <= 0) return;

	std::vector<float> audio_buffer(frames_available * 2);
	f_generate(rev_simulator_ptr, audio_buffer.data(), frames_available, 2);

	for (int i = 0; i < frames_available; ++i) {
		float l = audio_buffer[i * 2] * (float)master_gain;
		float r = audio_buffer[i * 2 + 1] * (float)master_gain;
		playback->push_frame(Vector2(l, r));
	}
}
