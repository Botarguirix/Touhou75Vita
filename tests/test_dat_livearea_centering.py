"""Geometry checks use synthetic RGBA pixels; no game assets are bundled."""
import sys
import unittest
from pathlib import Path
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from prepare_dat_livearea import centered_sprite


class DatCenteringTests(unittest.TestCase):
    def sprite(self, size=(512, 512), rectangle=(100, 30, 360, 480)):
        image = Image.new('RGBA', size)
        image.paste((250, 120, 90, 255), rectangle)
        return image

    def test_empty_alpha_rejected(self):
        with self.assertRaisesRegex(ValueError, 'empty character alpha'):
            centered_sprite(Image.new('RGBA', (10, 10)), (280, 158))

    def test_offcenter_source_becomes_centered(self):
        _, placement = centered_sprite(self.sprite(), (280, 158), 6)
        self.assertEqual(placement['source_bounds'], (100, 30, 360, 480))
        self.assertLessEqual(abs(placement['left'] - placement['right']), 1)
        self.assertLessEqual(abs(placement['top'] - placement['bottom']), 1)

    def test_icon_keeps_padding_and_opaque_canvas(self):
        image, placement = centered_sprite(self.sprite(), (256, 256), 12)
        self.assertEqual(image.size, (256, 256))
        self.assertEqual(image.getchannel('A').getextrema(), (255, 255))
        self.assertGreaterEqual(min(placement[k] for k in ['left', 'right', 'top', 'bottom']), 12)

    def test_wide_sprite_fits_gate_without_stretching(self):
        _, placement = centered_sprite(self.sprite(rectangle=(5, 20, 500, 100)), (280, 158), 6)
        width, height = placement['scaled_size']
        self.assertLessEqual(width, 268)
        self.assertLessEqual(height, 146)
        self.assertLessEqual(abs(width * 80 - height * 495), 495)

    def test_odd_size_rounding_at_most_one_pixel(self):
        _, placement = centered_sprite(self.sprite(), (281, 159), 6)
        for a, b in [('left', 'right'), ('top', 'bottom')]:
            self.assertLessEqual(abs(placement[a] - placement[b]), 1)


if __name__ == '__main__':
    unittest.main()
