#!/usr/bin/env python3
"""Store lossless PNG evidence while retaining raw device BMPs locally."""
from pathlib import Path
import sys
from PIL import Image

folder = Path(sys.argv[1])
for source in sorted(folder.glob('*.bmp')):
    with Image.open(source) as image:
        output = source.with_suffix('.png')
        image.save(output)
        with Image.open(output) as restored:
            assert image.mode == restored.mode and image.tobytes() == restored.tobytes()
        print(f'{source.name} -> {output.name}: identical decoded pixels')
