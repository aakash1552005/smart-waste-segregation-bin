# Model training

The waste classifier was trained in Google Colab. The notebook covers dataset
loading, preprocessing, model architecture, training, evaluation, and export
of `waste_classifier.h5` and `class_names.json`.

**Colab notebook:** [paste your Colab share link here]

## Classes

```json
["wet", "dry", "plastic", "biomedical", "recyclable"]
```

## Regenerating the model

1. Open the Colab notebook (link above).
2. Run all cells top to bottom.
3. Download the exported `waste_classifier.h5` and `class_names.json` from
   the final cell.
4. Place both files in `server/` (see [server/README](../server/README.md)),
   replacing the existing ones.

## Input format expected by the server

- Image size: 224x224
- RGB, normalized to `[0, 1]` (divide pixel values by 255)
- Model output: softmax over 5 classes, in the order listed above
