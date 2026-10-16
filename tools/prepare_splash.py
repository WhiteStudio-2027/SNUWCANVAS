"""Render the borderless loading screen asset without opening a window."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
ROOT = Path(__file__).resolve().parents[1]
def main():
    image = Image.new("RGB", (1200, 720), "#111112")
    with Image.open(ROOT / "snuw_canvas.ico") as source:
        mark = source.convert("RGBA").resize((160, 160), Image.Resampling.LANCZOS)
        image.paste(mark, (520, 220), mark)
    draw = ImageDraw.Draw(image)
    font = ImageFont.truetype(str(ROOT / "assets/fonts/NotoSansKR-Bold.ttf"), 46)
    draw.text((600, 430), "SNUWCAVNAS", font=font, fill="#eeeeef", anchor="mm")
    image.save(ROOT / "assets/splash.png", optimize=True)
if __name__ == "__main__":
    main()
