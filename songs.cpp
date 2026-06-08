// ======================================================
// HOW TO COMPILE & RUN
// ======================================================
//
// macOS / Linux:
//     g++ main.cpp -std=c++17 -O2 -o midi_gen
//     ./midi_gen
//
// Windows (MinGW):
//     g++ main.cpp -std=c++17 -O2 -o midi_gen.exe
//     midi_gen.exe
//
// Output:
//     64bar_groove_0.mid
//     64bar_groove_1.mid
//     64bar_groove_2.mid
//
// ======================================================

#include <fstream>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <string>
#include <cstdlib>
#include <ctime>

using namespace std;

const int TPQN = 480;

// -------------------------
// write big-endian
// -------------------------
void writeBE(ofstream& f, uint32_t value, int bytes) {
    for (int i = bytes - 1; i >= 0; --i)
        f.put((value >> (i * 8)) & 0xFF);
}

// -------------------------
// MIDI var length
// -------------------------
void writeVarLen(vector<uint8_t>& track, uint32_t value) {
    uint32_t buffer = value & 0x7F;

    while ((value >>= 7)) {
        buffer <<= 8;
        buffer |= ((value & 0x7F) | 0x80);
    }

    while (true) {
        track.push_back(buffer & 0xFF);
        if (buffer & 0x80)
            buffer >>= 8;
        else
            break;
    }
}

struct Event {
    uint32_t tick;
    bool on;
    uint8_t note;
    uint8_t velocity;
};

// -------------------------
// generate one song
// -------------------------
void generateSong(const string& filename, int seed) {

    srand(seed);

    vector<Event> events;

    const int bars = 64;
    const int beatsPerBar = 4;

    // HI-HATS (8th notes)
    for (int bar = 0; bar < bars; ++bar) {
        for (int step = 0; step < 8; ++step) {

            uint32_t tick =
                bar * beatsPerBar * TPQN +
                step * (TPQN / 2);

            int vel = 70 + rand() % 30;

            events.push_back({tick, true, 42, (uint8_t)vel});
            events.push_back({tick + 60, false, 42, 0});
        }
    }

    // KICK (1 and 4)
    for (int bar = 0; bar < bars; ++bar) {

        uint32_t b1 = bar * beatsPerBar * TPQN;
        uint32_t b4 = bar * beatsPerBar * TPQN + 3 * TPQN;

        events.push_back({b1, true, 36, 110});
        events.push_back({b1 + 120, false, 36, 0});

        if (rand() % 100 < 90) {
            events.push_back({b4, true, 36, 100});
            events.push_back({b4 + 120, false, 36, 0});
        }
    }

    // SNARE (every 3rd beat)
    int totalBeats = bars * beatsPerBar;

    for (int beat = 1; beat <= totalBeats; ++beat) {

        if (beat % 3 == 0) {

            uint32_t tick = (beat - 1) * TPQN;

            int vel = 110 + rand() % 15;

            events.push_back({tick, true, 38, (uint8_t)vel});
            events.push_back({tick + 120, false, 38, 0});
        }
    }

    // sort events
    sort(events.begin(), events.end(),
        [](const Event& a, const Event& b) {
            return a.tick < b.tick;
        });

    vector<uint8_t> track;
    uint32_t lastTick = 0;

    // tempo 120 BPM
    track.insert(track.end(),
        {0x00,0xFF,0x51,0x03,0x07,0xA1,0x20});

    auto writeEvent = [&](const Event& e) {

        uint32_t delta = e.tick - lastTick;
        lastTick = e.tick;

        writeVarLen(track, delta);

        if (e.on) {
            track.push_back(0x99);
            track.push_back(e.note);
            track.push_back(e.velocity);
        } else {
            track.push_back(0x89);
            track.push_back(e.note);
            track.push_back(0);
        }
    };

    for (auto& e : events)
        writeEvent(e);

    track.insert(track.end(),
        {0x00,0xFF,0x2F,0x00});

    ofstream midi(filename, ios::binary);

    midi.write("MThd", 4);
    writeBE(midi, 6, 4);
    writeBE(midi, 0, 2);
    writeBE(midi, 1, 2);
    writeBE(midi, TPQN, 2);

    midi.write("MTrk", 4);
    writeBE(midi, (uint32_t)track.size(), 4);

    midi.write((char*)track.data(), track.size());

    midi.close();
}

// -------------------------
// main: ONLY 3 FILES
// -------------------------
int main() {

    for (int i = 0; i < 3; i++) {

        string filename =
            "64bar_groove_" +
            to_string(i) + ".mid";

        generateSong(filename, time(nullptr) + i);
    }

    return 0;
}
