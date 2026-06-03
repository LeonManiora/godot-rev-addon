#ifndef GODOT_REV_ENGINE_H
#define GODOT_REV_ENGINE_H

#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/audio_stream_player3d.hpp>
#include <godot_cpp/classes/audio_stream_generator_playback.hpp>
#include <godot_cpp/classes/audio_stream_generator.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {

class GodotREVEngine : public Node3D {
    GDCLASS(GodotREVEngine, Node3D);

private:
    double rpminfluence = 0.0;
    double throttle_input = 0.0;
    int current_gear = 1;
    double master_gain = 1.0;

    // Pointer auf den Crankcase-Simulator im C++ Speicher
    void* rev_simulator_ptr = nullptr;
    int64_t dll_handle = 0;

    // Die Funktionszeiger deklarieren wir jetzt als generische Funktions-Pointer
    void* (*f_create)() = nullptr;
    void (*f_destroy)(void*) = nullptr;
    bool (*f_load)(void*, const uint8_t*, uint32_t) = nullptr;
    void (*f_update)(void*, float, float, float, int) = nullptr;
    void (*f_generate)(void*, float*, uint32_t, uint32_t) = nullptr;

    AudioStreamPlayer3D* generator_player = nullptr;
    Ref<AudioStreamGeneratorPlayback> playback;

protected:
    static void _bind_methods();

public:
    GodotREVEngine();
    ~GodotREVEngine();

    void _process(double delta) override;
    
    void set_rpminfluence(double value) { rpminfluence = value; }
    double get_rpminfluence() const { return rpminfluence; }
    void set_throttle(double value) { throttle_input = value; }
    double get_throttle() const { return throttle_input; }
    
    bool init_rev_library();
    bool load_rev_model(const PackedByteArray& model_bytes);
    void start_audio();
};

}

#endif
