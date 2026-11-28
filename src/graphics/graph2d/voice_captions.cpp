#include "common.h"
#include "typedefs.h"
#include "graphics/graph2d/voice_captions.h"

#include <algorithm>
#include <array>
#include <cstdint>

#include "enums.h"
#include "graphics/graph2d/message.h"
#include "graphics/mov/movie.h"
#include "iop/adpcm/iopadpcm.h"
#include "iop/se/voice.h"
#include "mikupan/io/mikupan_file_c.h"
#include "mikupan/rendering/mikupan_font_textures.h"
#include "mikupan/rendering/mikupan_renderer.h"
#include "os/eeiop/adpcm/ea_cmd.h"

namespace
{
struct VoiceCaption
{
    int file;
    int start;
    int end;
    const char* first;
    const char* second;
};

struct SoundEffectCaption
{
    int sample;
    const char* text;
};

#include "voice_captions_zh_hans.inc"

const VoiceCaption* FindVoiceCaption(int file, int64_t time)
{
    auto cue = std::lower_bound(kVoiceCaptions.begin(), kVoiceCaptions.end(), file,
        [](const VoiceCaption& caption, int id) { return caption.file < id; });
    for (; cue != kVoiceCaptions.end() && cue->file == file && cue->start <= time; ++cue)
    {
        if (time < cue->end)
            return &*cue;
    }
    return nullptr;
}

const char* FindSoundEffectCaption(int sample)
{
    const auto cue = std::lower_bound(kSoundEffectCaptions.begin(), kSoundEffectCaptions.end(), sample,
        [](const SoundEffectCaption& caption, int id) { return caption.sample < id; });
    return cue != kSoundEffectCaptions.end() && cue->sample == sample ? cue->text : nullptr;
}

}

void DrawVoiceCaption()
{
    if (!MikuPan_IsJapaneseGameData() || !MikuPanFonts::UsesChineseGlyphGrid()
        || movie_wrk.play_event_sta != 0)
        return;

    const int status = EAGetRetStat();
    const int count = EAGetRetCount();
    const int file = EAGetRetTune();
    // Each ADPCM interrupt consumes 0x800 bytes per channel, or 3584 samples at 48 kHz.
    const auto* caption = status >= ADPCM_STAT_PLAY && status <= ADPCM_STAT_RTRANS
        ? FindVoiceCaption(file, int64_t{count} * 3584 * 1000 / 48000) : nullptr;

    const bool tape = caption && file >= AVT000_OGATA0_STR && file <= AVT007_TAKAMINE1_STR;
    const float preferred_y = tape ? (caption->second ? 382.0f : 372.0f) : 400.0f;
    // Reserve room for a 24-pixel glyph, the shadow and a gap above the native box.
    float y = std::min(preferred_y, GetLowerMessageBoxTop() - 33.0f);
    constexpr float line_height = 28.0f;
    MikuPan_BeginLate2DOverlayQueue();
    if (caption)
    {
        if (caption->second)
        {
            DrawNativeVoiceCaptionLine(caption->second, y);
            y -= line_height;
        }
        DrawNativeVoiceCaptionLine(caption->first, y);
        y -= line_height;
    }
    if (!tape)
    {
        int active[24];
        const int count = VoiceGetActiveSoundEffects(active, std::size(active));
        std::sort(active, active + count);
        const int* end = std::unique(active, active + count);
        for (const int* sample = active; sample != end && y >= 140; ++sample)
        {
            if (const char* text = FindSoundEffectCaption(*sample))
            {
                DrawNativeVoiceCaptionLine(text, y);
                y -= line_height;
            }
        }
    }
    MikuPan_EndLate2DOverlayQueue();
}
