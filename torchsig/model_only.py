import shutil
import matplotlib.pyplot as plt
import cv2
import os
import yaml
from ultralytics import YOLO


#==============================================================================#

ROOT = "./datasets/nosleep"

# Constants
FFT_SIZE    = 1024
NUM_IQ_SAMPLES_DATASET = FFT_SIZE ** 2
CLASS_LIST = ["64qam", "fm","ofdm-2048","16fsk"]
NUM_CLASSES = len(CLASS_LIST)
NUM_TRAIN   = 10000
NUM_VAL     = 2000
EPOCHS      = 50
BATCH_SIZE  = 8
WORKERS     = 1
PATIENCE    = 10


#==============================================================================#

# Metadata
CONFIG_NAME = "nosleep.yaml"
classes = dict(enumerate(CLASS_LIST))

#==============================================================================#

# CV model used
MODEL_FILEPATH = "yolov8x.pt"
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
    project="nosleep1",
    name="nosleep1",
    verbose=True,
    cos_lr=True,
    cache=False,
    single_cls=False,
    profile=False,
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
