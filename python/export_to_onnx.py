#!/usr/bin/env python
import argparse
import tensorflow as tf
import tf2onnx

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--model", default="enhanced_nn_model.keras")
    parser.add_argument("--out", default="enhanced_nn_model.onnx")
    args = parser.parse_args()
    
    model = tf.keras.models.load_model(args.model)
    
    # Export only the main output
    spec = (tf.TensorSpec((None, len(FEATURES)), tf.float32, name="input"),) # type: ignore
    tf2onnx.convert.from_keras(
        model,
        input_signature=spec,
        opset=17,
        output_path=args.out,
        outputs=['main']
    )
    print(f"ONNX model saved to {args.out}")

if __name__ == "__main__":
    main()