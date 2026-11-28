"""Rebuild the Simplified Chinese puzzle labels from the bundled font.

Run with Pillow installed: python tools/puzzle_labels.py
The atlas regions mirror the Japanese sprite rectangles in ev_spcl.cpp.
"""

from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
SCALE = 3
SIZE = 512
FONT = ROOT / "resources/fonts/LXGWWenKai-Medium.ttf"
ATLAS = Image.new("RGBA", (SIZE * SCALE, SIZE * SCALE))


def paint(region, text, *, size=20, left=2, top=None):
    x, y, width, height = region
    image = Image.new("RGBA", (width * SCALE, height * SCALE))
    draw = ImageDraw.Draw(image)
    font = ImageFont.truetype(str(FONT), size * SCALE)
    box = draw.textbbox((0, 0), text, font=font)
    line_width, line_height = box[2] - box[0], box[3] - box[1]
    if line_width > (width - left - 2) * SCALE or line_height > (height - 2) * SCALE:
        raise ValueError((region, text))
    margin = (height * SCALE - line_height) // 2 if top is None else top * SCALE
    origin = (left * SCALE - box[0], margin - box[1])
    draw.text((origin[0] + SCALE, origin[1] + SCALE), text, font=font, fill=(20, 15, 9, 210))
    draw.text(origin, text, font=font, fill=(217, 203, 171, 255))
    ATLAS.alpha_composite(image, (x * SCALE, y * SCALE))


def main():
    # Star board control sheet and remaining-move sprites.
    paint((0, 0, 170, 26), "选择石块")
    paint((0, 26, 170, 26), "移动石块")
    paint((0, 54, 170, 25), "离开石板")
    paint((0, 79, 170, 24), "还原石块")
    paint((0, 104, 170, 24), "离开石板")
    paint((1, 133, 183, 49), "还剩", size=14, left=0, top=15)
    paint((121, 184, 96, 36), "步", size=18, left=1)
    for number, x, y in ((1, 58, 90), (2, 30, 90), (3, 2, 90), (4, 86, 52),
                         (5, 58, 52), (6, 30, 52), (7, 2, 52)):
        paint((x, 132 + y, 24, 36), str(number), size=20)

    # The cancel row becomes an undo while a digit has been entered.
    paint((256, 0, 80, 25), "按下")
    paint((256, 25, 128, 26), "离开这里")
    paint((256, 51, 128, 26), "选择文字")
    paint((256, 77, 128, 25), "退回一位")

    for row, text in enumerate(("选择人偶", "拿起人偶", "离开这里")):
        paint((256 + 43, 128 + row * 26, 120, 26), text)

    for offset, text in ((0, "选择位置"), (27, "放置佛像"), (54, "离开这里"),
                         (80, "退出选择"), (100, "选择佛像")):
        paint((256, 216 + offset, 140, 24), text, size=19)

    for row, text in enumerate(("选择蜡烛", "点燃", "恢复初始状态", "离开这里")):
        paint((0, 267 + row * 26, 159, 25), text)

    for row, text in enumerate(("选择梵字", "按下", "离开这里")):
        paint((0, 380 + row * 27, 130, 26), text)

    out = ROOT / "resources/text/puzzle-labels-zh-Hans.png"
    ATLAS.resize((SIZE, SIZE), Image.Resampling.LANCZOS).save(out, optimize=True)


if __name__ == "__main__":
    main()
