#include "godot_rev_engine.h"
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/os.hpp>
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
    if (dll_handle != 0) {
        OS::get_singleton()->unload_dynamic_library(dll_handle);
    }
}

bool GodotREVEngine::init_rev_library() {
    String path = "res://addons/crankcase/REVRuntime.dll";
    
    // NATIVE GODOT-BRÜCKE: Lädt die DLL absolut plattformunabhängig über Godots C++ API!
    Error err = OS.get_singleton()->load_dynamic_library(path, dll_handle);
    if (err != OK || dll_handle == 0) {
        UtilityFunctions::push_error("[REV Engine] REVRuntime.dll konnte nicht über Godot-OS geladen werden!");
        return false;
    }

    // Godots eigener Funktions-Abtaster (Ersetzt das fehlerhafte Windows GetProcAddress)
    OS::get_singleton()->get_dynamic_library_symbol_handle(dll_handle, "REV_CreateSimulator", *(void***)&f_create);
    OS::get_singleton()->get_dynamic_library_symbol_handle(dll_handle, "REV_DestroySimulator", *(void***)&f_destroy);
    OS::get_singleton()->get_dynamic_library_symbol_handle(dll_handle, "REV_LoadModelData", *(void***)&f_load);
    OS::get_singleton()->get_dynamic_library_symbol_handle(dll_handle, "REV_Update", *(void***)&f_update);
    OS::get_singleton()->get_dynamic_library_symbol_handle(dll_handle, "REV_GenerateAudio", *(void***)&f_generate);

    if (!f_create || !f_destroy || !f_load || !f_update || !f_generate) {
        UtilityFunctions::push_error("[REV Engine] DLL-Symbole unvollständig geladen!");
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

    // Wir erstellen ein einfaches C++ Array im Speicher
    float* audio_buffer = new float[frames_available * 2];
    
    f_generate(rev_simulator_ptr, audio_buffer, frames_available, 2);

    for (int i = 0; i < frames_available; ++i) {
        float l = audio_buffer[i * 2] * (float)master_gain;
        float r = audio_buffer[i * 2 + 1] * (float)master_gain;
        playback->push_frame(Vector2(l, r));
    }

    delete[] audio_buffer; // Speicher wieder freigeben
}
