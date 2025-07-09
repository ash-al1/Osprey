import shutil
import matplotlib.pyplot as plt
import cv2
import os
import yaml
from ultralytics import YOLO


#==============================================================================#

ROOT = "./datasets/wideband"

# Constants
FFT_SIZE    = 512
NUM_IQ_SAMPLES_DATASET = FFT_SIZE ** 2
CLASS_LIST = ["16qam","8gmsk", "32qam", "bpsk","qpsk"]
NUM_CLASSES = len(CLASS_LIST)
NUM_TRAIN   = 20000
NUM_VAL     = 4000
EPOCHS      = 500
BATCH_SIZE  = 16
WORKERS     = 1
PATIENCE    = 50


#==============================================================================#

# Metadata
CONFIG_NAME = "wideband_detector_yolo.yaml"
classes = dict(enumerate(CLASS_LIST))

#==============================================================================#

# CV model used
MODEL_FILEPATH = "/home/ic3/gits/Osprey/torchsig/yolo/sig4/weights/best.pt"
model = YOLO(MODEL_FILEPATH)

# Train
results = model.train(
    data=CONFIG_NAME,
    epochs=EPOCHS,
    batch=BATCH_SIZE,
    imgsz=FFT_SIZE,
    patience=PATIENCE,
    workers=WORKERS,
    val = True,
    plots = True,
    device="cuda",
    project="yolo",
    name="new",
    verbose=True,
    cos_lr=True,
    warmup_epochs=5,
    warmup_momentum=0.8,
    warmup_bias_lr=0.01,
    cache=False,
    single_cls=False,
    profile=False,
    lr0=0.0005,
    lrf=0.001,
    optimizer="SGD",
    momentum=0.9,
    dropout=0.2,
    label_smoothing=0.1,
    mosaic=0.0,
    close_mosaic=50,
    deterministic=True,
    degrees=0,
    flipud=0.0,
    shear=0.0,
    perspective=0.0,
    hsv_h=0.0,
    hsv_s=0.0,
    hsv_v=0.0,
    translate=0.0,
    scale=0.0,
    copy_paste=0.0,
    fliplr=0.0,
    box=7.5,
    cls=1.5,
    dfl=1.5,
    weight_decay=0.001,
    save=True,
    save_period=25,
)

print(f"done, check {ROOT}")


results_img = cv2.imread(os.path.join(results.save_dir, "results.png"))
plt.figure(figsize = (10,20))
plt.imshow(results_img)


label = cv2.imread(os.path.join(results.save_dir, "val_batch1_labels.jpg"))
pred = cv2.imread(os.path.join(results.save_dir, "val_batch1_pred.jpg"))

f, ax = plt.subplots(1, 2, figsize=(15, 9))
ax[0].imshow(label)
ax[0].set_title("Label")
ax[1].imshow(pred)
ax[1].set_title("Prediction")
plt.show()
