#include "common.h"
#include "typedefs.h"
#include "enums.h"
#include "ig_camra.h"

#include "graphics/graph2d/message.h"
#include "graphics/graph3d/sglib.h"
#include "ingame/menu/ig_file.h"
#include "ingame/menu/ig_menu.h"
#include "ingame/menu/item.h"
#include "main/glob.h"
#include "mikupan/io/mikupan_controller.h"
#include "mikupan/io/mikupan_file.h"
#include "graphics/graph2d/tim2.h"
#include "mikupan/rendering/mikupan_font_textures.h"
#include "os/eeiop/eese.h"
#include "outgame/mode_slct.h"

namespace
{
struct CameraCell
{
    short x, y, w, h;
};

constexpr int camera_choice_y = 385;
constexpr int japanese_choice_spacing = 120;

CameraCell CameraCommandCell(int index, bool japanese)
{
    return japanese ? CameraCell{32, short(100 + index * 38), 122, 30}
                    : CameraCell{32, short(120 + index * 50), 122, 42};
}

CameraCell CameraFilmCell(int index, bool japanese)
{
    return japanese ? CameraCell{184, short(118 + index * 30), 182, 28}
                    : CameraCell{short(204 + (index / 2) * 205), short(129 + (index % 2) * 49), 197, 45};
}

CameraCell CameraAbilityCell(int index, bool special, bool japanese)
{
    return japanese ? CameraCell{short((special ? 406 : 184) + index * 42), 282, 40, 40}
                    : CameraCell{short(204 + index * 46), 271, 44, 44};
}

CameraCell CameraChoiceCell(int index, bool japanese)
{
    if (japanese && MikuPanFonts::UsesChineseGlyphGrid())
    {
        return {short(320 - japanese_choice_spacing / 2 - 35 + index * japanese_choice_spacing),
                camera_choice_y - 3, 70, 29};
    }
    return japanese ? CameraCell{short(212 + index * japanese_choice_spacing), 382, 90, 27}
                    : CameraCell{short(178 + index * 132), 380, 120, 27};
}
}

static void MikuPan_CameraMenuMouseInput(void)
{
    MikuPan_LegacyMouseState mouse;
    if (!MikuPan_LegacyMouseGetState(&mouse))
    {
        return;
    }

    const bool japanese = MikuPan_IsJapaneseGameData();
    for (int i = 0; i < 4; i++)
    {
        const auto cell = CameraCommandCell(i, japanese);
        if (!MikuPan_LegacyMouseHit(cell.x, cell.y, cell.w, cell.h))
        {
            continue;
        }
        if (mouse.moved && cmr_menu_wrk.csr[0] != i)
        {
            cmr_menu_wrk.csr[0] = i;
            SeStartFix(SE_CSR0, 0, 0x1000, 0x1000, 1);
        }
        if (mouse.left_pressed)
        {
            cmr_menu_wrk.csr[0] = i;
            MikuPan_QueueLegacyControllerButton(MIKUPAN_CONTROLLER_CROSS);
        }
        break;
    }
    if (mouse.right_pressed)
    {
        MikuPan_QueueLegacyControllerButton(MIKUPAN_CONTROLLER_TRIANGLE);
    }
}

static void MikuPan_CameraFilmMouseInput(void)
{
    MikuPan_LegacyMouseState mouse;
    if (!MikuPan_LegacyMouseGetState(&mouse))
    {
        return;
    }

    const bool japanese = MikuPan_IsJapaneseGameData();
    for (int i = 0; i < 4; i++)
    {
        const auto cell = CameraFilmCell(i, japanese);
        if (!MikuPan_LegacyMouseHit(cell.x, cell.y, cell.w, cell.h))
        {
            continue;
        }
        if (mouse.moved && cmr_menu_wrk.csr[1] != i)
        {
            cmr_menu_wrk.csr[1] = i;
            SeStartFix(SE_CSR0, 0, 0x1000, 0x1000, 1);
        }
        if (mouse.left_pressed)
        {
            cmr_menu_wrk.csr[1] = i;
            MikuPan_QueueLegacyControllerButton(MIKUPAN_CONTROLLER_CROSS);
        }
        break;
    }
    if (mouse.right_pressed)
    {
        MikuPan_QueueLegacyControllerButton(MIKUPAN_CONTROLLER_TRIANGLE);
    }
}

static void MikuPan_CameraYesNoMouseInput(void)
{
    MikuPan_LegacyMouseState mouse;
    if (!MikuPan_LegacyMouseGetState(&mouse))
    {
        return;
    }

    const bool japanese = MikuPan_IsJapaneseGameData();
    for (int i = 0; i < 2; i++)
    {
        const auto cell = CameraChoiceCell(i, japanese);
        if (!MikuPan_LegacyMouseHit(cell.x, cell.y, cell.w, cell.h))
        {
            continue;
        }
        if (mouse.moved && cmr_menu_wrk.csr[5] != i)
        {
            cmr_menu_wrk.csr[5] = i;
            SeStartFix(SE_CSR0, 0, 0x1000, 0x1000, 1);
        }
        if (mouse.left_pressed)
        {
            cmr_menu_wrk.csr[5] = i;
            MikuPan_QueueLegacyControllerButton(MIKUPAN_CONTROLLER_CROSS);
        }
        break;
    }

    if (mouse.right_pressed)
    {
        MikuPan_QueueLegacyControllerButton(MIKUPAN_CONTROLLER_TRIANGLE);
    }
}

int camera_power_up_point[3][3] = {
    {  4000, 12000, 24000 },
    {  2000,  6000, 12000 },
    {  6000, 18000, 36000 },
};
int camera_sub_abi_point[5] = {
    16000, 18000, 12000, 20000, 14000,
};
int camera_spe_abi_point[5] = {
    42000, 64000, 80000, 74000, 96000,
};

static CAM_DSP cam_dsp;

#define PI 3.1415927f

extern FLSH_CORE flsh;
extern FLSH_CORE flsh2;

void CameraCustomNewgameInit()
{
    cam_custom_wrk = {};

    cam_custom_wrk.set_spe = 0xff;
    cam_custom_wrk.set_sub = 0xff;

    cam_custom_wrk.func_sub[0] = 1;
    cam_custom_wrk.func_sub[1] = 1;
    cam_custom_wrk.func_sub[2] = 1;
    cam_custom_wrk.func_sub[3] = 1;
    cam_custom_wrk.func_sub[4] = 1;

    cam_custom_wrk.func_spe[0] = 0;
    cam_custom_wrk.func_spe[1] = 0;
    cam_custom_wrk.func_spe[2] = 0;
    cam_custom_wrk.func_spe[3] = 0;
    cam_custom_wrk.func_spe[4] = 0;
}

void CameraCustomInit()
{
    int i;
    u_char *pow[3] = {
        &cam_custom_wrk.charge_range, &cam_custom_wrk.charge_speed, &cam_custom_wrk.charge_max,
    };

    cam_dsp = {};
    cmr_menu_wrk = {};

    cmr_menu_wrk.mode = 4;

    cam_dsp.pow_bak[0] = *pow[0];
    cam_dsp.pow_bak[1] = *pow[1];
    cam_dsp.pow_bak[2] = *pow[2];

    yw2d.pad_lock = 1;

    cam_dsp.arw_cnt = 0;

    cam_dsp.film_or_perform = 0;
    cam_dsp.bonus_or_special = 0;

    cam_dsp.pfm_max[0] = 0;
    cam_dsp.pfm_max[1] = 0;
    cam_dsp.pfm_max[2] = 0;

    flsh = {};
    flsh2 = {};

    for (i = 0; i < 5; i++)
    {
        cam_dsp.sb_flsh[i] = 20;
        cam_dsp.ex_flsh[i] = 20;
    }
}

void CameraCustomMain()
{
    static char err;

    if (yw2d.pad_lock == 0 && cmr_menu_wrk.yn_mode != 0)
    {
        MikuPan_CameraYesNoMouseInput();
    }

    switch(cmr_menu_wrk.mode)
    {
    case 4:
        CameraCustomMenuSlct(&err);
    break;
    case 0:
        CameraCustomFilm(&err);
    break;
    case 1:
        CameraCustomPowerUp(&err);
    break;
    case 2:
        CameraCustomSubAbility(&err);
    break;
    case 3:
        CameraCustomSpecialAbility(&err);
    break;
    }

    CameraDspCtrl(err);
}

void CameraCustomMenuSlct(char *err)
{
    if (yw2d.pad_lock == 0)
    {
        MikuPan_CameraMenuMouseInput();
        if (TRIANGLE_PRESSED() == 1)
        {
            SeStartFix(SE_CANCEL, 0, 0x1000, 0x1000, 1);

            yw2d.out_mode_cnt = 11;
        }
        else if (
            DPAD_DOWN_PRESSED() == 1 ||
            (DPAD_DOWN_PRESSED() > 25 && (DPAD_DOWN_PRESSED() % 5) == 1) ||
            Ana2PadDirCnt(2) == 1 ||
            (Ana2PadDirCnt(2) > 25 && (Ana2PadDirCnt(2) % 5) == 1)
        )
        {
            if (cmr_menu_wrk.csr[0] <= 2)
            {
                cmr_menu_wrk.csr[0]++;
            }
            else
            {
                cmr_menu_wrk.csr[0] = 0;
            }

            SeStartFix(SE_CSR0, 0, 0x1000, 0x1000, 1);
        }
        else if (
            DPAD_UP_PRESSED() == 1 ||
            (DPAD_UP_PRESSED() > 25 && (DPAD_UP_PRESSED() % 5) == 1) ||
            Ana2PadDirCnt(0) == 1 ||
            (Ana2PadDirCnt(0) > 25 && (Ana2PadDirCnt(0) % 5) == 1)
        )
        {
            if (cmr_menu_wrk.csr[0] != 0)
            {
                cmr_menu_wrk.csr[0]--;
            }
            else
            {
                cmr_menu_wrk.csr[0] = 3;
            }

            SeStartFix(SE_CSR0, 0, 0x1000, 0x1000, 1);
        }
        else if (SQUARE_PRESSED() == 1 || CROSS_PRESSED() == 1)
        {
            cmr_menu_wrk.yn_mode = 0;
            cmr_menu_wrk.mode = cmr_menu_wrk.csr[0];

            SeStartFix(SE_CLIC, 0, 0x1000, 0x1000, 1);
        }
    }
}

void CameraCustomFilm(char *err)
{
    const bool japanese = MikuPan_IsJapaneseGameData();
    if (yw2d.pad_lock == 0)
    {
        MikuPan_CameraFilmMouseInput();
        if (TRIANGLE_PRESSED() == 1)
        {
            SeStartFix(SE_CANCEL, 0, 0x1000, 0x1000, 1);

            if (*err != 0)
            {
                *err = 0;
            }
            else
            {
                cmr_menu_wrk.mode = 4;
            }

            return;
        }

        if (SQUARE_PRESSED() == 1 || CROSS_PRESSED() == 1)
        {
            if (*err != 0)
            {
                SeStartFix(SE_CLIC, 0, 0x1000, 0x1000, 1);

                *err = 0;
            }
            else
            {
                if (poss_item[cmr_menu_wrk.csr[1] + 1] != 0)
                {
                    if (plyr_wrk.film_no != cmr_menu_wrk.csr[1] + 1)
                    {
                        plyr_wrk.film_no = cmr_menu_wrk.csr[1] + 1;

                        SeStartFix(SE_SEALING, 0, 0x1000, 0x1000, 1);
                    }
                    else
                    {
                        *err = 2;

                        SeStartFix(SE_CLIC_NO, 0, 0x1000, 0x1000, 1);
                    }
                }
                else if (flm_exp_flg[cmr_menu_wrk.csr[1] + 1] == 0)
                {
                    SeStartFix(SE_CLIC_NO, 0, 0x1000, 0x1000, 1);
                }
                else
                {
                    *err = 4;

                    SeStartFix(SE_CLIC_NO, 0, 0x1000, 0x1000, 1);
                }
            }
        }
        else if (
            (
                DPAD_DOWN_PRESSED() == 1 ||
                (DPAD_DOWN_PRESSED() > 25 && (DPAD_DOWN_PRESSED() % 5) == 1) ||
                Ana2PadDirCnt(2) == 1 ||
                (Ana2PadDirCnt(2) > 25 && (Ana2PadDirCnt(2) % 5) == 1)
            ) || (!japanese && (
                DPAD_UP_PRESSED() == 1 ||
                (DPAD_UP_PRESSED() > 25 && (DPAD_UP_PRESSED() % 5) == 1) ||
                Ana2PadDirCnt(0) == 1 ||
                (Ana2PadDirCnt(0) > 25 && (Ana2PadDirCnt(0) % 5) == 1)
            ))
        )
        {
            *err = 0;

            cmr_menu_wrk.csr[1] = japanese ? (cmr_menu_wrk.csr[1] + 1) % 4
                                          : cmr_menu_wrk.csr[1] ^ 1;

            SeStartFix(SE_CSR0, 0, 0x1000, 0x1000, 1);
        }
        else if (japanese && (
            DPAD_UP_PRESSED() == 1 ||
            (DPAD_UP_PRESSED() > 25 && (DPAD_UP_PRESSED() % 5) == 1) ||
            Ana2PadDirCnt(0) == 1 ||
            (Ana2PadDirCnt(0) > 25 && (Ana2PadDirCnt(0) % 5) == 1)))
        {
            *err = 0;
            cmr_menu_wrk.csr[1] = (cmr_menu_wrk.csr[1] + 3) % 4;
            SeStartFix(SE_CSR0, 0, 0x1000, 0x1000, 1);
        }
        else if (!japanese && (
            (
                DPAD_LEFT_PRESSED() == 1 ||
                (DPAD_LEFT_PRESSED() > 25 && (DPAD_LEFT_PRESSED() % 5) == 1) ||
                Ana2PadDirCnt(3) == 1 ||
                (Ana2PadDirCnt(3) > 25 && (Ana2PadDirCnt(3) % 5) == 1)
            ) || (
                DPAD_RIGHT_PRESSED() == 1 ||
                (DPAD_RIGHT_PRESSED() > 25 && (DPAD_RIGHT_PRESSED() % 5) == 1) ||
                Ana2PadDirCnt(1) == 1 ||
                (Ana2PadDirCnt(1) > 25 && (Ana2PadDirCnt(1) % 5) == 1)
            )
        ))
        {
            *err = 0;

            cmr_menu_wrk.csr[1] ^= 2;

            SeStartFix(SE_CSR0, 0, 0x1000, 0x1000, 1);
        }
    }
}

void CameraCustomPowerUp(char *err)
{
    u_char *pow[3] = {
        &cam_custom_wrk.charge_range, &cam_custom_wrk.charge_speed, &cam_custom_wrk.charge_max,
    };

    u_int uVar1;
    u_long *puVar2;
    int bVar3;
    char cVar4;
    u_short uVar5;
    u_short uVar6;
    int se_no;

    if (cmr_menu_wrk.yn_mode == 0)
    {
        if (yw2d.pad_lock == 0)
        {
            if (TRIANGLE_PRESSED() == 1)
            {
                SeStartFix(SE_CANCEL, 0, 0x1000, 0x1000, 1);
                if (*err == 0)
                {
                    cmr_menu_wrk.mode = 4;
                }
                else
                {
                    *err = 0;
                }
            }
            else if (
                DPAD_DOWN_PRESSED() == 1 ||
                (DPAD_DOWN_PRESSED() > 25 && (DPAD_DOWN_PRESSED() % 5) == 1) ||
                Ana2PadDirCnt(2) == 1 ||
                (Ana2PadDirCnt(2) > 25 && (Ana2PadDirCnt(2) % 5) == 1)
            )
            {
                *err = 0;
                if (cmr_menu_wrk.csr[2] <= 1)
                {
                    cmr_menu_wrk.csr[2] = cmr_menu_wrk.csr[2] + 1;
                }
                else
                {
                    cmr_menu_wrk.csr[2] = 0;
                }
                SeStartFix(SE_CSR0, 0, 0x1000, 0x1000, 1);
            }
            else if  (
                DPAD_UP_PRESSED() == 1 ||
                (DPAD_UP_PRESSED() > 25 && (DPAD_UP_PRESSED() % 5) == 1) ||
                Ana2PadDirCnt(0) == 1 ||
                (Ana2PadDirCnt(0) > 25 && (Ana2PadDirCnt(0) % 5) == 1)
            )
            {
                *err = 0;
                if (cmr_menu_wrk.csr[2] != 0)
                {
                    cmr_menu_wrk.csr[2]--;
                }
                else
                {
                    cmr_menu_wrk.csr[2] = 2;
                }
                SeStartFix(SE_CSR0, 0, 0x1000, 0x1000, 1);
            }
            else if (SQUARE_PRESSED() == 1 || CROSS_PRESSED() == 1)
            {
                if (*err != 0)
                {
                    SeStartFix(SE_CLIC, 0, 0x1000, 0x1000, 1);
                    *err = 0;
                }
                else if (*pow[cmr_menu_wrk.csr[2]] != 3)
                {
                    if (cam_custom_wrk.point >= camera_power_up_point[cmr_menu_wrk.csr[2]][*pow[cmr_menu_wrk.csr[2]]])
                    {
                        cmr_menu_wrk.yn_mode = 1;

                        cmr_menu_wrk.csr[5] = 0;

                        SeStartFix(SE_CLIC, 0, 0x1000, 0x1000, 1);
                    }
                    else
                    {
                        *err = 1;

                        SeStartFix(SE_CLIC_NO, 0, 0x1000, 0x1000, 1);
                    }
                }
            }
        }
    }
    else if (cmr_menu_wrk.yn_mode == 1)
    {
        if (yw2d.pad_lock == 0)
        {
            if (TRIANGLE_PRESSED() == 1)
            {
                SeStartFix(SE_CANCEL, 0, 0x1000, 0x1000, 1);

                if (*err != 0)
                {
                    *err = 0;
                }
                else
                {
                    cmr_menu_wrk.yn_mode = 0;
                }
            }
            else if (DPAD_LEFT_PRESSED() == 1 || Ana2PadDirCnt(3) == 1 || DPAD_RIGHT_PRESSED() == 1 || Ana2PadDirCnt(1) == 1)
            {
                *err = 0;

                if (cmr_menu_wrk.csr[5] != 0)
                {
                    cmr_menu_wrk.csr[5] = 0;
                }
                else
                {
                    cmr_menu_wrk.csr[5] = 1;
                }

                SeStartFix(SE_CSR0, 0, 0x1000, 0x1000, 1);
            }
            else if (SQUARE_PRESSED() == 1 || CROSS_PRESSED() == 1)
            {
                SeStartFix(SE_CLIC, 0, 0x1000, 0x1000, 1);

                if (*err != 0)
                {
                    *err = 0;
                }
                else if (cmr_menu_wrk.csr[5] != 0)
                {
                    cmr_menu_wrk.yn_mode = 0;
                }
                else
                {
                    cam_custom_wrk.point = cam_custom_wrk.point - camera_power_up_point[cmr_menu_wrk.csr[2]][*pow[cmr_menu_wrk.csr[2]]];

                    (*pow[cmr_menu_wrk.csr[2]])++;

                    cmr_menu_wrk.yn_mode = 0;

                    if (*pow[cmr_menu_wrk.csr[2]] == 3)
                    {
                        cam_dsp.pfm_max[cmr_menu_wrk.csr[2]] = 20;
                    }
                }
            }
        }
    }
}

void CameraCustomSubAbility(char *err)
{
    int i;

    if (cmr_menu_wrk.yn_mode == 0)
    {
        if (yw2d.pad_lock == 0)
        {
            if (TRIANGLE_PRESSED() == 1)
            {
                SeStartFix(SE_CANCEL, 0, 0x1000, 0x1000, 1);

                if (*err != 0)
                {
                    *err = 0;
                }
                else
                {
                    cmr_menu_wrk.mode = 4;
                }
            }
            else if (
                DPAD_RIGHT_PRESSED() == 1 ||
                (DPAD_RIGHT_PRESSED() > 25 && (DPAD_RIGHT_PRESSED() % 5) == 1) ||
                Ana2PadDirCnt(1) == 1 ||
                (Ana2PadDirCnt(1) > 25 && (Ana2PadDirCnt(1) % 5) == 1)
            )
            {
                *err = 0;

                if (cmr_menu_wrk.csr[3] <= 3)
                {
                    cmr_menu_wrk.csr[3]++;
                }
                else
                {
                    cmr_menu_wrk.csr[3] = 0;
                }

                SeStartFix(SE_CSR0, 0, 0x1000, 0x1000, 1);
            }
            else if (
                DPAD_LEFT_PRESSED() == 1 ||
                (DPAD_LEFT_PRESSED() > 25 && (DPAD_LEFT_PRESSED() % 5) == 1) ||
                Ana2PadDirCnt(3) == 1 ||
                (Ana2PadDirCnt(3) > 25 && (Ana2PadDirCnt(3) % 5) == 1)
            )
            {
                *err = 0;

                if (cmr_menu_wrk.csr[3] != 0)
                {
                    cmr_menu_wrk.csr[3]--;
                }
                else
                {
                    cmr_menu_wrk.csr[3] = 4;
                }

                SeStartFix(SE_CSR0, 0, 0x1000, 0x1000, 1);
            }
            else if (SQUARE_PRESSED() == 1 || CROSS_PRESSED() == 1)
            {
                if (*err != 0)
                {
                    *err = 0;

                    SeStartFix(SE_CLIC, 0, 0x1000, 0x1000, 1);

                    return;
                }

                if (cam_custom_wrk.func_sub[cmr_menu_wrk.csr[3]] == 0)
                {
                    SeStartFix(SE_CLIC_NO, 0, 0x1000, 0x1000, 1);
                }
                else if (cam_custom_wrk.func_sub[cmr_menu_wrk.csr[3]] == 1)
                {
                    if (cam_custom_wrk.point >= camera_sub_abi_point[cmr_menu_wrk.csr[3]])
                    {
                        cmr_menu_wrk.yn_mode = 1;
                        cmr_menu_wrk.csr[5] = 0;

                        SeStartFix(SE_CLIC, 0, 0x1000, 0x1000, 1);
                    }
                    else
                    {
                        *err = 1;

                        SeStartFix(SE_CLIC_NO, 0, 0x1000, 0x1000, 1);
                    }
                }
                else if (cam_custom_wrk.func_sub[cmr_menu_wrk.csr[3]] == 2)
                {
                    cmr_menu_wrk.yn_mode = 2;
                    cmr_menu_wrk.csr[5] = 0;

                    SeStartFix(SE_CLIC, 0, 0x1000, 0x1000, 1);

                }
                else if (cam_custom_wrk.func_sub[cmr_menu_wrk.csr[3]] == 3)
                {
                    cmr_menu_wrk.yn_mode = 3;
                    cmr_menu_wrk.csr[5] = 0;

                    SeStartFix(SE_CLIC, 0, 0x1000, 0x1000, 1);
                }
            }
        }
    }
    else if (cmr_menu_wrk.yn_mode == 1)
    {
        if (yw2d.pad_lock == 0)
        {
            if (TRIANGLE_PRESSED() == 1)
            {
                SeStartFix(SE_CANCEL, 0, 0x1000, 0x1000, 1);

                if (*err != 0)
                {
                    *err = 0;
                }
                else
                {
                    cmr_menu_wrk.yn_mode = 0;
                }
            }
            else if (DPAD_LEFT_PRESSED() == 1 || Ana2PadDirCnt(3) == 1 || DPAD_RIGHT_PRESSED() == 1 || Ana2PadDirCnt(1) == 1)
            {
                *err = 0;

#ifdef MATCHING_DECOMP
                do {} while(0); // HACK: mitigates assembler short loop bug
#endif

                if (cmr_menu_wrk.csr[5] != 0)
                {
                    cmr_menu_wrk.csr[5] = 0;
                }
                else
                {
                    cmr_menu_wrk.csr[5] = 1;
                }

                SeStartFix(SE_CSR0, 0, 0x1000, 0x1000, 1);
            }
            else if (SQUARE_PRESSED() == 1 || CROSS_PRESSED() == 1)
            {
                SeStartFix(SE_CLIC, 0, 0x1000, 0x1000, 1);

                if (*err != 0)
                {
                    *err = 0;
                }
                else if (cmr_menu_wrk.csr[5] != 0)
                {
                    cmr_menu_wrk.yn_mode = 0;
                }
                else
                {
                    cam_custom_wrk.func_sub[cmr_menu_wrk.csr[3]] = 2;
                    cam_custom_wrk.point = cam_custom_wrk.point - camera_sub_abi_point[cmr_menu_wrk.csr[3]];

                    cmr_menu_wrk.yn_mode = 2;
                }
            }
        }
    }
    else if (cmr_menu_wrk.yn_mode == 2)
    {
        if (yw2d.pad_lock == 0)
        {
            if (TRIANGLE_PRESSED() == 1)
            {
                SeStartFix(SE_CANCEL, 0, 0x1000, 0x1000, 1);

                if (*err != 0)
                {
                    *err = 0;
                }
                else
                {
                    cmr_menu_wrk.yn_mode = 0;
                }
            }
            else if (DPAD_LEFT_PRESSED() == 1 || Ana2PadDirCnt(3) == 1 || DPAD_RIGHT_PRESSED() == 1 || Ana2PadDirCnt(1) == 1)
            {
                *err = 0;

                if (cmr_menu_wrk.csr[5] != 0)
                {
                    cmr_menu_wrk.csr[5] = 0;
                }
                else
                {
                    cmr_menu_wrk.csr[5] = 1;
                }

                SeStartFix(SE_CSR0, 0, 0x1000, 0x1000, 1);
            }
            else if (SQUARE_PRESSED() == 1 || CROSS_PRESSED() == 1)
            {
                SeStartFix(SE_CLIC, 0, 0x1000, 0x1000, 1);

                if (*err != 0)
                {
                    *err = 0;
                }
                else if (cmr_menu_wrk.csr[5] != 0)
                {
                    cmr_menu_wrk.yn_mode = 0;
                }
                else
                {
                    for (i = 0; i < 5; i++)
                    {
                        if (cam_custom_wrk.func_sub[i] == 3)
                        {
                            cam_custom_wrk.func_sub[i] = 2;
                        }
                    }

                    cam_custom_wrk.func_sub[cmr_menu_wrk.csr[3]] = 3;
                    cam_custom_wrk.set_sub = cmr_menu_wrk.csr[3];

                    cmr_menu_wrk.yn_mode = 0;
                }
            }
        }
    }
    else if (cmr_menu_wrk.yn_mode == 3)
    {
        if (yw2d.pad_lock == 0)
        {
            if (TRIANGLE_PRESSED() == 1)
            {
                SeStartFix(SE_CANCEL, 0, 0x1000, 0x1000, 1);

                if (*err != 0)
                {
                    *err = 0;
                }
                else
                {
                    cmr_menu_wrk.yn_mode = 0;
                }
            }
            else if (DPAD_LEFT_PRESSED() == 1 || Ana2PadDirCnt(3) == 1 || DPAD_RIGHT_PRESSED() == 1 || Ana2PadDirCnt(1) == 1)
            {
                *err = 0;

                if (cmr_menu_wrk.csr[5] != 0)
                {
                    cmr_menu_wrk.csr[5] = 0;
                }
                else
                {
                    cmr_menu_wrk.csr[5] = 1;
                }

                SeStartFix(SE_CSR0, 0, 0x1000, 0x1000, 1);
            }
            else if (SQUARE_PRESSED() == 1 || CROSS_PRESSED() == 1)
            {
                SeStartFix(SE_CLIC, 0, 0x1000, 0x1000, 1);

                if (*err != 0)
                {
                    *err = 0;
                }
                else if (cmr_menu_wrk.csr[5] != 0)
                {
                    cmr_menu_wrk.yn_mode = 0;
                }
                else
                {
                    cam_custom_wrk.func_sub[cmr_menu_wrk.csr[3]] = 2;
                    cam_custom_wrk.set_sub = 0xff;

                    cmr_menu_wrk.yn_mode = 0;
                }
            }
        }
    }
}

void CameraCustomSpecialAbility(char *err)
{
    int i;

    if (cmr_menu_wrk.yn_mode == 0)
    {
        if (yw2d.pad_lock == 0)
        {
            if (TRIANGLE_PRESSED() == 1)
            {
                SeStartFix(SE_CANCEL, 0, 0x1000, 0x1000, 1);

                if (*err != 0)
                {
                    *err = 0;
                }
                else
                {
                    cmr_menu_wrk.mode = 4;
                }

                return;
            }

            if (
                DPAD_RIGHT_PRESSED() == 1 ||
                (DPAD_RIGHT_PRESSED() > 25 && (DPAD_RIGHT_PRESSED() % 5) == 1) ||
                Ana2PadDirCnt(1) == 1 ||
                (Ana2PadDirCnt(1) > 25 && (Ana2PadDirCnt(1) % 5) == 1)
            )
            {
                *err = 0;

                if (cmr_menu_wrk.csr[4] <= 3)
                {
                    cmr_menu_wrk.csr[4]++;
                }
                else
                {
                    cmr_menu_wrk.csr[4] = 0;
                }

                SeStartFix(SE_CSR0, 0, 0x1000, 0x1000, 1);
            }
            else if (
                DPAD_LEFT_PRESSED() == 1 ||
                (DPAD_LEFT_PRESSED() > 25 && (DPAD_LEFT_PRESSED() % 5) == 1) ||
                Ana2PadDirCnt(3) == 1 ||
                (Ana2PadDirCnt(3) > 25 && (Ana2PadDirCnt(3) % 5) == 1)
            )
            {
                *err = 0;

                if (cmr_menu_wrk.csr[4] != 0)
                {
                    cmr_menu_wrk.csr[4]--;
                }
                else
                {
                    cmr_menu_wrk.csr[4] = 4;
                }

                SeStartFix(SE_CSR0, 0, 0x1000, 0x1000, 1);
            }
            else if (SQUARE_PRESSED() == 1 || CROSS_PRESSED() == 1)
            {
                if (*err != 0)
                {
                    *err = 0;

                    SeStartFix(SE_CLIC, 0, 0x1000, 0x1000, 1);

                    return;
                }

                if (cam_custom_wrk.func_spe[cmr_menu_wrk.csr[4]] == 0)
                {
                    SeStartFix(SE_CLIC_NO, 0, 0x1000, 0x1000, 1);
                }
                else if (cam_custom_wrk.func_spe[cmr_menu_wrk.csr[4]] == 1)
                {
                    if (camera_spe_abi_point[cmr_menu_wrk.csr[4]] <= cam_custom_wrk.point)
                    {
                        SeStartFix(SE_CLIC, 0, 0x1000, 0x1000, 1);

                        cmr_menu_wrk.csr[5] = 0;
                        cmr_menu_wrk.yn_mode = 1;
                    }
                    else
                    {
                        *err = 1;

                        SeStartFix(SE_CLIC_NO, 0, 0x1000, 0x1000, 1);
                    }
                }
                else if (cam_custom_wrk.func_spe[cmr_menu_wrk.csr[4]] == 2)
                {
                    cmr_menu_wrk.csr[5] = 0;
                    cmr_menu_wrk.yn_mode = 2;

                    SeStartFix(SE_CLIC, 0, 0x1000, 0x1000, 1);
                }
                else if (cam_custom_wrk.func_spe[cmr_menu_wrk.csr[4]] == 3)
                {
                    cmr_menu_wrk.csr[5] = 0;
                    cmr_menu_wrk.yn_mode = 3;

                    SeStartFix(SE_CLIC, 0, 0x1000, 0x1000, 1);
                }

            }
        }
    }
    else if (cmr_menu_wrk.yn_mode == 1)
    {
        if (yw2d.pad_lock == 0)
        {
            if (TRIANGLE_PRESSED() == 1)
            {
                SeStartFix(SE_CANCEL, 0, 0x1000, 0x1000, 1);

                if (*err != 0)
                {
                    *err = 0;
                }
                else
                {
                    cmr_menu_wrk.yn_mode = 0;
                }

                return;
            }

            if (DPAD_LEFT_PRESSED() == 1 || Ana2PadDirCnt(3) == 1 || DPAD_RIGHT_PRESSED() == 1 || Ana2PadDirCnt(1) == 1)
            {
                *err = 0;

#ifdef MATCHING_DECOMP
                do {} while(0); // HACK: mitigates assembler short loop bug
#endif

                if (cmr_menu_wrk.csr[5] != 0)
                {
                    cmr_menu_wrk.csr[5] = 0;
                }
                else
                {
                    cmr_menu_wrk.csr[5] = 1;
                }

                SeStartFix(SE_CSR0, 0, 0x1000, 0x1000, 1);
            }
            else if (SQUARE_PRESSED() == 1 || CROSS_PRESSED() == 1)
            {
                SeStartFix(SE_CLIC, 0, 0x1000, 0x1000, 1);

                if (*err != 0)
                {
                    *err = 0;
                }
                else if (cmr_menu_wrk.csr[5] != 0)
                {
                    cmr_menu_wrk.yn_mode = 0;
                }
                else
                {
                    cam_custom_wrk.func_spe[cmr_menu_wrk.csr[4]] = 2;
                    cam_custom_wrk.point = cam_custom_wrk.point - camera_spe_abi_point[cmr_menu_wrk.csr[4]];

                    cmr_menu_wrk.yn_mode = 2;
                }
            }
        }
    }
    else if (cmr_menu_wrk.yn_mode == 2)
    {
        if (yw2d.pad_lock == 0)
        {
            if (TRIANGLE_PRESSED() == 1)
            {
                SeStartFix(SE_CANCEL, 0, 0x1000, 0x1000, 1);

                if (*err != 0)
                {
                    *err = 0;
                }
                else
                {
                    cmr_menu_wrk.yn_mode = 0;
                }

                return;
            }

            if (DPAD_LEFT_PRESSED() == 1 || Ana2PadDirCnt(3) == 1 || DPAD_RIGHT_PRESSED() == 1 || Ana2PadDirCnt(1) == 1)
            {
                *err = 0;

                if (cmr_menu_wrk.csr[5] != 0)
                {
                    cmr_menu_wrk.csr[5] = 0;
                }
                else
                {
                    cmr_menu_wrk.csr[5] = 1;
                }

                SeStartFix(SE_CSR0, 0, 0x1000, 0x1000, 1);
            }
            else if (SQUARE_PRESSED() == 1 || CROSS_PRESSED() == 1)
            {
                SeStartFix(SE_CLIC, 0, 0x1000, 0x1000, 1);

                if (*err != 0)
                {
                    *err = 0;
                }
                else if (cmr_menu_wrk.csr[5] != 0)
                {
                    cmr_menu_wrk.yn_mode = 0;
                }
                else
                {
                    for (i = 0; i < 5; i++)
                    {
                        if (cam_custom_wrk.func_spe[i] == 3)
                        {
                            cam_custom_wrk.func_spe[i] = 2;
                        }
                    }

                    cmr_menu_wrk.yn_mode = 0;

                    cam_custom_wrk.func_spe[cmr_menu_wrk.csr[4]] = 3;
                    cam_custom_wrk.set_spe = cmr_menu_wrk.csr[4];
                }
            }
        }
    }
    else if (cmr_menu_wrk.yn_mode == 3)
    {
        if (yw2d.pad_lock == 0)
        {
            if (TRIANGLE_PRESSED() == 1)
            {
                SeStartFix(SE_CANCEL, 0, 0x1000, 0x1000, 1);

                if (*err != 0)
                {
                    *err = 0;
                }
                else
                {
                    cmr_menu_wrk.yn_mode = 0;
                }

                return;
            }

            if (DPAD_LEFT_PRESSED() == 1 || Ana2PadDirCnt(3) == 1 || DPAD_RIGHT_PRESSED() == 1 || Ana2PadDirCnt(1) == 1)
            {
                 *err = 0;

                if (cmr_menu_wrk.csr[5] != 0)
                {
                    cmr_menu_wrk.csr[5] = 0;
                }
                else
                {
                    cmr_menu_wrk.csr[5] = 1;
                }

                SeStartFix(SE_CSR0, 0, 0x1000, 0x1000, 1);
            }
            else if (SQUARE_PRESSED() == 1 || CROSS_PRESSED() == 1)
            {
                SeStartFix(SE_CLIC, 0, 0x1000, 0x1000, 1);

                if (*err != 0)
                {
                    *err = 0;
                }
                else if (cmr_menu_wrk.csr[5] != 0)
                {
                    cmr_menu_wrk.yn_mode = 0;
                }
                else
                {
                    cmr_menu_wrk.yn_mode = 0;

                    cam_custom_wrk.func_spe[cmr_menu_wrk.csr[4]] = 2;
                    cam_custom_wrk.set_spe = 0xff;
                }
            }
        }
    }
}

void CameraDspCtrl(u_char err)
{
    u_char msg;

    CameraModeInOut();

    msg = StrKndChk(err);

    CameraDsp(0, 0, yw2d.io_a[4], msg);

    PlayerStatusYW(yw2d.io_a[2]);

    DispCaption(18, yw2d.io_a[4]);

    CameraModeInOut2();
    CameraCntRenew();
}

void CameraDsp(short int pos_x, short int pos_y, u_char alp, u_char msg)
{
    const bool japanese = MikuPan_IsJapaneseGameData();
    int i;
    int j;
    int cost;
    int rgb; // STAB say u_char
    u_char flsh_alp[6];
    u_char use;
    u_char *pow[3] = {
        &cam_custom_wrk.charge_range, &cam_custom_wrk.charge_speed, &cam_custom_wrk.charge_max,
    };
    int cst_rgb;
    const int power_row_step = japanese ? 40 : 32;
    float arw_alp;

    PutSpriteYW(CAM_BACK_TEX, CAM_BACK_TEX, pos_x, pos_y, 0.0f, 0x404040, alp, 1.0f, 1.0f, 0, 0xff, 1, 0, 0);

    switch (cmr_menu_wrk.csr[0])
    {
    case 0:
        cam_dsp.film_or_perform = 0;
    break;
    case 1:
        cam_dsp.film_or_perform = 1;
    break;
    case 2:
        cam_dsp.bonus_or_special = 0;
    break;
    case 3:
         cam_dsp.bonus_or_special = 1;
    break;
    }

    FlashStarYW(&flsh, 64, 96, 90, 0);
    FlashStarYW(&flsh2, alp, (int)(alp / 2), 90, 0);

    switch(cmr_menu_wrk.mode)
    {
    case 4:
        flsh_alp[0] = flsh.alpha;
        flsh_alp[4] = 0;
        flsh_alp[1] = flsh_alp[2] = flsh_alp[3] = 0;
    break;
    case 0:
        flsh_alp[1] = flsh.alpha;
        flsh_alp[0] = 0x40;
        flsh_alp[3] = flsh_alp[4] = 0;
        flsh_alp[2] = 0;
    break;
    case 1:
        flsh_alp[2] = flsh.alpha;
        flsh_alp[0] = 0x40;
        flsh_alp[4] = 0;
        flsh_alp[1] = flsh_alp[3] = 0;
    break;
    case 2:
        flsh_alp[3] = flsh.alpha;
        flsh_alp[0] = 0x40;
        flsh_alp[4] = 0;
        flsh_alp[1] = flsh_alp[2] = 0;
    break;
    case 3:
        flsh_alp[4] = flsh.alpha;
        flsh_alp[0] = 0x40;
        flsh_alp[1] = flsh_alp[2] = flsh_alp[3] = 0;
    break;
    }

    cost = -1;

    switch (cmr_menu_wrk.mode)
    {
    case 1:
        if (*pow[cmr_menu_wrk.csr[2]] != 3)
        {
            cost = camera_power_up_point[cmr_menu_wrk.csr[2]][*pow[cmr_menu_wrk.csr[2]]];
        }
    break;
    case 2:
        if (cam_custom_wrk.func_sub[cmr_menu_wrk.csr[3]] == 1)
        {
            cost = camera_sub_abi_point[cmr_menu_wrk.csr[3]];
        }
    break;
    case 3:
        if (cam_custom_wrk.func_spe[cmr_menu_wrk.csr[4]] == 1)
        {
             cost = camera_spe_abi_point[cmr_menu_wrk.csr[4]];
        }
    break;
    }

    flsh_alp[5] = 0x40;

    if (cost != -1)
    {
        if (cam_custom_wrk.point >= cost)
        {
            flsh_alp[5] = flsh2.alpha;

            PutNumberYW(3, cost, japanese ? 34 : 521, japanese ? 296 : 289, 1.0f, 1.0f, 0x808080, alp, 0x5000, 5, 0);
        }
        else
        {
            PutNumberYW(3, cost, japanese ? 34 : 521, japanese ? 296 : 289, 1.0f, 1.0f, 0x808080, alp * 0.5f, 0x5000, 5, 0);
        }
    }

    if (japanese)
        WakuWaku(pos_x - 16, pos_y + 68, alp, 4, 0x2d);
    else
        WakuWaku2(pos_x - 15, pos_y, alp, 4, 0x37);

    PutSpriteYW(CAM_CMND_00, CAM_CMND_03, pos_x - (japanese ? 10 : 15), pos_y, 0.0f, 0x808080, alp, 1.0f, 1.0f, 0, 0xff, 1, 0, 0);

    DspMenuTitle(pos_x, pos_y, alp, 50, 7);

    CmnWindow(33, pos_x - (japanese ? 9 : 0), pos_y, alp, flsh_alp[5]);

    CLRtoRGB(&cst_rgb, flsh_alp[5], flsh_alp[5], flsh_alp[5]);

    PutSpriteYW(CAM_FONT_NXT, CAM_FONT_NXT, pos_x, pos_y, 0.0f, japanese ? 0x808080 : cst_rgb, alp, 1.0f, 1.0f, 0, 0xff, japanese ? 0 : 1, 0, 0);
    if (japanese)
    {
        const u_char cost_alpha = cost == -1 || cam_custom_wrk.point >= cost ? alp : alp / 2;
        PutStringYW(IGMSG_CAME_MSG, 1, 118, 300, 0x808080, cost_alpha, 0x5000, 0);
    }
    else
    {
        PutSpriteYW(CAM_WTTL_04, CAM_WTTL_04, pos_x, pos_y, 0.0f, 0x808080, alp, 1.0f, 1.0f, 0, 0xff, 0, 0, 0);
    }

    if (japanese || cam_dsp.film_or_perform == 0)
    {
        rgb = cmr_menu_wrk.csr[0] == 0 ? 0x80 : 0x40;

        CmnWindow(34, pos_x, pos_y, alp, rgb);

        PutSpriteYW(CAM_WTTL_00, CAM_WTTL_00, pos_x, pos_y, 0.0f, 0x808080, alp, 1.0f, 1.0f, 0, 0xff, 0, 0, 0);
    }
    if (japanese || cam_dsp.film_or_perform != 0)
    {
        rgb = cmr_menu_wrk.csr[0] == 1 ? 0x80 : 0x40;

        CmnWindow(35, pos_x, pos_y, alp, rgb);

        PutSpriteYW(CAM_WTTL_01, CAM_WTTL_01, pos_x, pos_y, 0.0f, 0x808080, alp, 1.0f, 1.0f, 0, 0xff, 0, 0, 0);
    }

    if (japanese || cam_dsp.bonus_or_special == 0)
    {
        rgb = cmr_menu_wrk.csr[0] == 2 ? 0x80 : 0x40;

        CmnWindow(36, pos_x, pos_y, alp, rgb);

        PutSpriteYW(CAM_WTTL_02, CAM_WTTL_02, pos_x, pos_y, 0.0f, 0x808080, alp, 1.0f, 1.0f, 0, 0xff, 0, 0, 0);
    }
    if (japanese || cam_dsp.bonus_or_special != 0)
    {
        rgb = cmr_menu_wrk.csr[0] == 3 ? 0x80 : 0x40;

        CmnWindow(37, pos_x, pos_y, alp, rgb);

        PutSpriteYW(CAM_WTTL_03, CAM_WTTL_03, pos_x, pos_y, 0.0f, 0x808080, alp, 1.0f, 1.0f, 0, 0xff, 0, 0, 0);
    }

    if (japanese || cam_dsp.film_or_perform == 0)
    {
        for (i = 0; i < 4; i++)
        {
            const int count = poss_item[i + 1];
            if (!count && !flm_exp_flg[i + 1])
                continue;

            const auto cell = CameraFilmCell(i, japanese);
            const int color = count || japanese ? 0x808080 : 0x404040;
            PutSpriteYW(CAM_FILM_00 + i, CAM_FILM_00 + i, pos_x, pos_y, 0.0f, japanese ? 0x808080 : 0x404040, alp, 1.0f, 1.0f, 0, 0xff, 1, 0, 0);
            PutStringYW(IGMSG_ITEM_NAME, i + 1, pos_x + cell.x + (japanese ? 40 : 2),
                        pos_y + cell.y + 2, color, alp, 0x2000, 0);
            if (count || !japanese)
            {
                PutStringYW(IGMSG_CAME_MSG, 0, pos_x + cell.x + (japanese ? 132 : 147),
                            pos_y + cell.y + (japanese ? 5 : 26), color, alp, 0x2000, 0);
                PutNumberYW(3, count, pos_x + cell.x + (japanese ? 148 : 163),
                            pos_y + cell.y + (japanese ? 1 : 22), japanese ? 0.92f : 1.0f,
                            1.0f, color, alp, 0x2000, 2, 1);
            }
            if (!count)
                PolySquareYW(pos_x + cell.x, pos_y + cell.y, cell.w, cell.h, 0,
                             alp / 2, 1.0f, 1.0f, japanese ? 0x3000 : 0x2000, 0, 0, 0);
        }
    }
    if (japanese || cam_dsp.film_or_perform != 0)
    {
        for (i = 0; i < 3; i++)
        {
            if (japanese)
            {
                // The equivalent US sprite IDs also belong to other menus.
                SPRT_DAT label = {0x2005b8a625422cc5, 1, u_short(111 + i * 38), 105, 38,
                                  380 + pos_x, 118 + pos_y + i * power_row_step, 0x19000, alp};
                DISP_SPRT display;
                CopySprDToSpr(&display, &label);
                DispSprD(&display);
                continue;
            }

            PutSpriteYW(CAM_FONT2_00 + i, CAM_FONT2_00 + i, pos_x, pos_y, 0.0f, 0x808080, alp, 1.0f, 1.0f, 0, 0xff, 1, 0, 0);

            if (*pow[i] == 3)
            {
                PutSpriteYW(CAM_FONT_11, CAM_FONT_11, pos_x - 5, pos_y + i * 32 - 7, 0.0f, 0x808080, alp, 1.0f, 1.0f, 0, 0xff, 1, 0, 0);
                PutSpriteYW(CAM_FONT_11, CAM_FONT_11, pos_x - 5, pos_y + i * 32 - 7, 0.0f, 0x808080, alp * BeMax(i), 1.0f, 1.0f, 0, 0xff, 1, 1, 0);
            }
            else
            {
                for (j = 0; j < 5; j++)
                {
                    arw_alp = alp * NeonAlp(5, 60, 10, 50, j, &cam_dsp.arw_cnt);

                    PutSpriteYW(CAM_FONT_10, CAM_FONT_10, pos_x + j * 15, pos_y + i * 32, 0.0f, 0x808080, arw_alp * 0.25f, 1.0f, 1.0f, 0, 0xff, 1, 0, 0);
                    PutSpriteYW(CAM_FONT_10, CAM_FONT_10, pos_x + j * 15, pos_y + i * 32, 0.0f, 0x808080, arw_alp * 0.5f, 1.0f, 1.0f, 0, 0xff, 1, 1, 0);
                }
            }
        }

        if (cam_dsp.arw_cnt < 0xfff)
        {
            cam_dsp.arw_cnt++;
        }
        else
        {
            cam_dsp.arw_cnt = 0;
        }

        for (i = 0; i < 3; i++)
        {
            PutSpriteYW(CAM_FONT_LV, CAM_FONT_LV, pos_x, pos_y + i * power_row_step, 0.0f, 0x808080, alp, 1.0f, 1.0f, 0, 0xff, 1, 0, 0);

            for (j = 0; j < 4; j++)
            {
                if (j < *pow[i] + 1)
                {
                    PutSpriteYW(CAM_BALL_ONN, CAM_BALL_ONN, pos_x + j * 22, pos_y + i * power_row_step, 0.0f, 0x808080, alp, 1.0f, 1.0f, 0, 0xff, 1, 0, 0);
                }
                else
                {
                    PutSpriteYW(CAM_BALL_OFF, CAM_BALL_OFF, pos_x + j * 22, pos_y + i * power_row_step, 0.0f, 0x808080, alp, 1.0f, 1.0f, 0, 0xff, 1, 0, 0);
                }
            }
        }

        for (i = 0; i < 3; i++)
        {
            if (*pow[i] != cam_dsp.pow_bak[i])
            {
                cam_dsp.pow_flr[i][*pow[i]] = 20;
                cam_dsp.pow_bak[i] = *pow[i];
            }

            for (j = 0; j < 4; j++)
            {
                if (cam_dsp.pow_flr[i][j] != 0)
                {
                    u_char alpha; // not in STAB

                    cam_dsp.pow_flr[i][j]--;

                    alpha = alp * SgSinf((cam_dsp.pow_flr[i][j] * 3.1415927f) / 20.0f);

                    PutSpriteYW(CAM_BALL_ONN, CAM_BALL_ONN, pos_x + j * 22, pos_y + i * power_row_step, 0.0f, 0x808080, alpha, 1.0f, 1.0f, 0, 0xff, 1, 1, 0);
                    PutSpriteYW(CAM_BALL_FLR, CAM_BALL_FLR, pos_x + j * 22, pos_y + i * power_row_step, 0.0f, 0x808080, alpha, 1.0f, 1.0f, 0, 0xff, 1, 1, 0);
                }
            }
        }
    }

    if (japanese || cam_dsp.bonus_or_special == 0)
    {
        for (i = 0; i < 5; i++)
        {
            switch (cam_custom_wrk.func_sub[i])
            {
            case 0:
                // do nothing ...
            break;
            case 1:
            {
                const auto cell = CameraAbilityCell(i, false, japanese);
                PutSpriteYW(CAM_FONT_00 + i, CAM_FONT_00 + i, pos_x, pos_y, 0.0f, 0x404040, alp, 1.0f, 1.0f, 0, 0xff, 1, 0, 0);
                PolySquareYW(pos_x + cell.x, pos_y + cell.y, cell.w, cell.h, 0, alp / 2, 1.0f, 1.0f, 0xf000, 0, 0, 0);
            }
            break;
            case 2:
                PutSpriteYW(CAM_FONT_00 + i, CAM_FONT_00 + i, pos_x, pos_y, 0.0f, 0x808080, flsh2.alpha, 1.0f, 1.0f, 0, 0xff, 1, 0, 0);

                cam_dsp.sb_flsh[i] = 0;
            break;
            case 3:
                if (cam_dsp.sb_flsh[i] < 20)
                {
                    PutSpriteYW(CAM_FONT_00 + i, CAM_FONT_00 + i, pos_x, pos_y, 0.0f, 0x808080, alp, 1.0f, 1.0f, 0, 0xff, 1, 0, 0);

                    use = alp * SgSinf((cam_dsp.sb_flsh[i] * 3.1415927f) / 20.0f);

                    PutSpriteYW(CAM_FONT_00 + i, CAM_FONT_00 + i, pos_x, pos_y, 0.0f, 0x808080, use, 1.0f, 1.0f, 0, 0xff, 1, 1, 0);

                    cam_dsp.sb_flsh[i]++;
                }
                else
                {
                    PutSpriteYW(CAM_FONT_00 + i, CAM_FONT_00 + i, pos_x, pos_y, 0.0f, 0x808080, flsh2.alpha, 1.0f, 1.0f, 0, 0xff, 1, 0, 0);
                }
            break;
            }
        }
    }
    if (japanese || cam_dsp.bonus_or_special != 0)
    {
        for (i = 0; i < 5; i++)
        {
            switch (cam_custom_wrk.func_spe[i])
            {
            case 0:
                // do nothing ...
            break;
            case 1:
            {
                const auto cell = CameraAbilityCell(i, true, japanese);
                PutSpriteYW(CAM_FONT_05 + i, CAM_FONT_05 + i, pos_x, pos_y, 0.0f, 0x404040, alp, 1.0f, 1.0f, 0, 0xff, 1, 0, 0);
                PolySquareYW(pos_x + cell.x, pos_y + cell.y, cell.w, cell.h, 0, alp / 2, 1.0f, 1.0f, 0xa000, 0, 0, 0);
            }
            break;
            case 2:
                PutSpriteYW(CAM_FONT_05 + i, CAM_FONT_05 + i, pos_x, pos_y, 0.0f, 0x808080, flsh2.alpha, 1.0f, 1.0f, 0, 0xff, 1, 0, 0);

                cam_dsp.ex_flsh[i] = 0;
            break;
            case 3:
                if (cam_dsp.ex_flsh[i] < 20)
                {
                    PutSpriteYW(CAM_FONT_05 + i, CAM_FONT_05 + i, pos_x, pos_y, 0.0f, 0x808080, alp, 1.0f, 1.0f, 0, 0xff, 1, 0, 0);

                    use = alp * SgSinf((cam_dsp.ex_flsh[i] * 3.1415927f) / 20.0f);

                    PutSpriteYW(CAM_FONT_05 + i, CAM_FONT_05 + i, pos_x, pos_y, 0.0f, 0x808080, use, 1.0f, 1.0f, 0, 0xff, 1, 1, 0);

                    cam_dsp.ex_flsh[i]++;
                }
                else
                {
                    PutSpriteYW(CAM_FONT_05 + i, CAM_FONT_05 + i, pos_x, pos_y, 0.0f, 0x808080, flsh2.alpha, 1.0f, 1.0f, 0, 0xff, 1, 0, 0);
                }
            break;
            }
        }
    }

    const auto cursor = [&](CameraCell cell, int index, int priority)
    {
        CmnCursol(pos_x + cell.x, pos_y + cell.y, cell.w, cell.h, flsh_alp[index], alp, priority);
    };
    cursor(CameraCommandCell(cmr_menu_wrk.csr[0], japanese), 0, 0xa000);
    cursor(CameraFilmCell(cmr_menu_wrk.csr[1], japanese), 1, japanese ? 0x3000 : 0x2000);
    cursor(japanese ? CameraCell{378, short(118 + cmr_menu_wrk.csr[2] * power_row_step), 236, 38}
                    : CameraCell{204, short(129 + cmr_menu_wrk.csr[2] * power_row_step), 402, 30}, 2, 0xa000);
    cursor(CameraAbilityCell(cmr_menu_wrk.csr[3], false, japanese), 3, 0xa000);
    cursor(CameraAbilityCell(cmr_menu_wrk.csr[4], true, japanese), 4, 0xa000);

    DrawMessageBox(0x19000, pos_x + 23, pos_y + 340, 594.0f, 84.0f, alp);

    if (cmr_menu_wrk.yn_mode != 0)
    {
        if (msg != 0xff)
        {
            PutStringYW(IGMSG_CAME_MSG, msg, 318, 356, 0x808080, alp, 0x5000, 1);
            if (!japanese || !PutChineseChoiceMessageYW(IGMSG_CAME_MSG, 2, cmr_menu_wrk.csr[5],
                                                       48, camera_choice_y, alp, 0x5000, japanese_choice_spacing))
            {
                const auto choice = CameraChoiceCell(cmr_menu_wrk.csr[5], japanese);
                PutStringYW(IGMSG_CAME_MSG, 2, japanese ? 318 : 48, camera_choice_y, 0x808080, alp, 0x5000, japanese ? 1 : 0);
                YesNoCrslOKR(0x14000, choice.x, choice.y, 0x808080, alp, japanese ? 1.5f : 2.0f);
            }
        }
    }
    else
    {
        if (msg != 0xff)
        {
            PutStringYW(IGMSG_CAME_MSG, msg, 48, 356, 0x808080, alp, 0x5000, 0);
        }
    }
}

CAMERA_MENU_WRK cmr_menu_wrk = {0};
FLSH_CORE flsh = {0};
FLSH_CORE flsh2 = {0};

char StrKndChk(u_char err)
{
    char str;

    str = -1;

    switch(cmr_menu_wrk.mode)
    {
    case 4:
        str = cmr_menu_wrk.csr[0] + 4;
    break;
    case 0:
        if (err != 0)
        {
            switch (err)
            {
            case 1:
                str = 3;
            break;
            case 2:
                str = 12;
            break;
            case 3:
                str = 13;
            break;
            case 4:
                str = 14;
            break;
            }
        }
        else if (cmr_menu_wrk.yn_mode != 0)
        {
            str = cmr_menu_wrk.csr[1] + 8;
        }
        else if (FilmPossChk(cmr_menu_wrk.csr[1]) != 0)
        {
            str = cmr_menu_wrk.csr[1] + 53;
        }
    break;
    case 1:
        if (err != 0)
        {
            switch (err)
            {
            case 1:
                str = 3;
            break;
            case 2:
                str = cmr_menu_wrk.csr[2] + 18;
            break;
            }
        }
        else if (cmr_menu_wrk.yn_mode != 0)
        {
            str = cmr_menu_wrk.csr[2] + 15;
        }
        else
        {
            str = cmr_menu_wrk.csr[2] + 57;
        }
    break;
    case 2:
        if (err != 0)
        {
            switch (err)
            {
            case 1:
                str = 3;
            break;
            case 2:
                str = 36;
            break;
            }
        }
        else if (cmr_menu_wrk.yn_mode != 0)
        {
            switch (cam_custom_wrk.func_sub[cmr_menu_wrk.csr[3]])
            {
            case 1:
                str = cmr_menu_wrk.csr[3] + 26;
            break;
            case 2:
                str = cmr_menu_wrk.csr[3] + 31;
            break;
            case 3:
                str = cmr_menu_wrk.csr[3] + 60;
            break;
            }
        }
        else if (cam_custom_wrk.func_sub[cmr_menu_wrk.csr[3]] != 0)
        {
            str = cmr_menu_wrk.csr[3] + 21;
        }
    break;
    case 3:
        if (err != 0)
        {
            switch (err)
            {
            case 1:
                str = 3;
            break;
            case 2:
                str = 52;
            break;
            }
        }
        else if (cmr_menu_wrk.yn_mode != 0)
        {
            switch (cam_custom_wrk.func_spe[cmr_menu_wrk.csr[4]])
            {
            case 1:
                str = cmr_menu_wrk.csr[4] + 42;
            break;
            case 2:
                str = cmr_menu_wrk.csr[4] + 47;
            break;
            case 3:
                str = cmr_menu_wrk.csr[4] + 65;
            break;
            }
        }
        else if (cam_custom_wrk.func_spe[cmr_menu_wrk.csr[4]] != 0)
        {
            str = cmr_menu_wrk.csr[4] + 37;
        }
    break;
    }

    return str;
}

char FilmPossChk(u_char knd)
{
    char rtrn;
    char chk;

    chk = knd + 1;

    if (flm_exp_flg[chk] != 0)
    {
        if (poss_item[chk] != 0)
        {
            rtrn = 2;
        }
        else
        {
            rtrn = 1;
        }
    }
    else
    {
        if (poss_item[chk] == 0)
        {
            rtrn = 0;
        }
        else
        {
            rtrn = -1;
        }
    }

    return rtrn;
}

void CameraModeInOut()
{
    int i;
    float fade_in;
    float fade_out;

    if (yw2d.inn_mode_cnt < 11)
    {
        fade_in = (yw2d.inn_mode_cnt * 128) / 10.0f;
        fade_out = (1.0f - yw2d.inn_mode_cnt / 10.0f) * 128.0f;

        yw2d.io_a[0] = 128.0f;
        yw2d.io_a[1] = fade_in;
        yw2d.io_a[2] = 128.0f;
        yw2d.io_a[3] = fade_out;
        yw2d.io_a[4] = fade_in;

        if (sys_wrk.game_mode != GAME_MODE_INGAME)
        {
            yw2d.io_a[2] = fade_in;

        }
        else
        {
            DspTopMenu(yw2d.io_x[3], yw2d.io_y[3], yw2d.io_a[3], 0);
            DspPlayData(yw2d.io_x[3], yw2d.io_y[3], yw2d.io_a[3], 1);
        }

        if (yw2d.inn_mode_cnt == 10)
        {
            yw2d.pad_lock = 0;
        }
    }
    else if (yw2d.out_mode_cnt != 0)
    {
        fade_in = (1.0f - (yw2d.out_mode_cnt - 1) / 10.0f) * 128.0f;
        fade_out = ((yw2d.out_mode_cnt - 1) * 128) / 10.0f;

        yw2d.io_a[0] = 128.0f;
        yw2d.io_a[1] = fade_out;
        yw2d.io_a[2] = 128.0f;
        yw2d.io_a[3] = fade_in;
        yw2d.io_a[4] = fade_out;

        if (sys_wrk.game_mode != GAME_MODE_INGAME)
        {
            yw2d.io_a[2] = fade_out;
        }
        else
        {
            DspTopMenu(yw2d.io_x[3], yw2d.io_y[3], yw2d.io_a[3], 0);
            DspPlayData(yw2d.io_x[3], yw2d.io_y[3], yw2d.io_a[3], 1);
        }

        yw2d.pad_lock = 1;

        if (yw2d.out_mode_cnt == 1)
        {
            ig_menu_wrk.mode = IGMENU_MODE_SLCT;

            IngameMenuModeSlctDispInit();

            yw2d.menu_io_flg = 0;
            yw2d.pad_lock = 0;
        }
    }
    else
    {
        for (i = 0; i < 7; i++)
        {
            XYAdefaultYW(i);
        }
    }
}

void CameraModeInOut2()
{
    if (yw2d.inn_mode_cnt < 11)
    {
        if (sys_wrk.game_mode == GAME_MODE_INGAME)
        {
            DspPlayData(yw2d.io_x[3], yw2d.io_y[3], yw2d.io_a[3], 2);
        }
    }
    else
    {
        if (yw2d.out_mode_cnt != 0 && sys_wrk.game_mode == GAME_MODE_INGAME)
        {
            DspPlayData(yw2d.io_x[3], yw2d.io_y[3], yw2d.io_a[3], 2);
        }
    }
}

void CameraCntRenew()
{
    if (yw2d.inn_mode_cnt < 65535)
    {
        yw2d.inn_mode_cnt++;
    }

    if (yw2d.out_mode_cnt != 0)
    {
        yw2d.out_mode_cnt--;
    }
}

int isCameraTopMenu()
{
    return cmr_menu_wrk.mode == 4;
}

void OutGameInitCamera()
{
    yw2d.inn_mode_cnt = 0;

    ItemInitAtMenuOpen();

    ig_menu_wrk.mode = 6;
}

int isCameraEnd()
{
    return ig_menu_wrk.mode != IGMENU_MODE_CAMERA;
}

float GetCamDispAlpha()
{
    if (yw2d.inn_mode_cnt != 0)
    {
        return yw2d.io_a[2];
    }

    return 0.0f;
}

void OpenCameraMenu()
{
    cribo.clear_info |= 0x4;
}

float NeonAlp(short int num, short int exe, short int dly, short int stp, short int no, short int *timer)
{
    float rad;
    short val;

    stp = (dly * (num - 1) + exe + stp);

    if (*timer >= stp)
    {
        *timer = 0;
    }

    val = no * dly;
    val = *timer - val;

    if (val >= 0 && exe > val)
    {
        rad = (val * (PI * 2)) / exe;
    }
    else
    {
        rad = 0.0f;
    }

    return (2.0f - (SgCosf(rad) + 1.0f)) * 0.5f;
}

float BeMax(u_char no)
{
    float per;

    if (cam_dsp.pfm_max[no] != 0)
    {
        per = SgSinf((cam_dsp.pfm_max[no] * PI) / 20.0f);

        cam_dsp.pfm_max[no]--;
    }
    else
    {
        per = 0.0f;
    }

    return per;
}
