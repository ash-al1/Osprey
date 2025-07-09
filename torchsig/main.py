import shutil
import os
import cv2
import requests
import yaml
import torch
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle
from tqdm.notebook import tqdm
from ultralytics import YOLO

from torchsig.transforms.dataset_transforms import Spectrogram
from torchsig.transforms.target_transforms import YOLOLabel
from torchsig.datasets.dataset_metadata import WidebandMetadata
from torchsig.datasets.wideband import NewWideband

#==============================================================================#

ROOT = "./datasets/wideband"

# Constants
FFT_SIZE    = 1024
NUM_IQ_SAMPLES_DATASET = FFT_SIZE ** 2
#CLASS_LIST  = ["16qam", "32qam", "64qam", "256qam", "1024qam", "bpsk", "qpsk",
#               "8gfsk"]
CLASS_LIST  = ["64qam", "256qam", "bpsk", "qpsk"]
NUM_CLASSES = len(CLASS_LIST)
NUM_TRAIN   = 20000
NUM_VAL     = 4000
EPOCHS      = 1
BATCH_SIZE  = 16
WORKERS     = 4
PATIENCE    = 20


#==============================================================================#

# IQ->Spectrogram
transforms        = [Spectrogram(fft_size=FFT_SIZE)]
target_transforms = [YOLOLabel()]

# Creates metadata of dataset to be generated
dataset_metadata = WidebandMetadata(
    num_iq_samples_dataset = NUM_IQ_SAMPLES_DATASET,
    fft_size = FFT_SIZE,
    impairment_level = 2,
    num_signals_max  = 2,
    class_list=CLASS_LIST,
    transforms=transforms,
    target_transforms=target_transforms,
)

# Dataset generation
wideband = NewWideband(dataset_metadata = dataset_metadata)

#==============================================================================#

data, label = wideband[0]
height, width = data.shape

# Write dataset to disk
def prepare_dataset(dataset, train: bool, root: str, start_index: int, stop_index: int):
    os.makedirs(root, exist_ok = True)
    train_path = "train" if train else "val"
    label_dir = f"{root}/labels/{train_path}"
    image_dir = f"{root}/images/{train_path}"
    os.makedirs(label_dir, exist_ok = True)
    os.makedirs(image_dir, exist_ok = True)

    for i in tqdm(range(start_index, stop_index), desc=f"Writing {train_path.title()} Dataset"):
        image, labels = dataset[i]
        filename_base = str(i).zfill(10)
        label_filename = f"{label_dir}/{filename_base}.txt"
        image_filename = f"{image_dir}/{filename_base}.png"

        with open(label_filename, "w") as f:
            f.write("\n".join(f"{x[0]} {x[1]} {x[2]} {x[3]} {x[4]}" for x in labels))
        cv2.imwrite(image_filename, image, [cv2.IMWRITE_PNG_COMPRESSION, 9])

if os.path.exists(ROOT):
    shutil.rmtree(ROOT)

# Train and validation datasets
prepare_dataset(wideband, train=True, root=ROOT, start_index=1,
                stop_index = NUM_TRAIN)
prepare_dataset(wideband, train=False, root=ROOT, start_index=NUM_TRAIN,
                stop_index = NUM_TRAIN + NUM_VAL)

#==============================================================================#

# Metadata
CONFIG_NAME = "wideband_detector_yolo.yaml"
classes = dict(enumerate(CLASS_LIST))

# YOLO configuration
yolo_config = dict(
    path = "wideband",
    train = "images/train",
    val = "images/val",
    nc = NUM_CLASSES,
    names = classes
)

with open(CONFIG_NAME, 'w+') as file:
    yaml.dump(yolo_config, file, default_flow_style=False)

#==============================================================================#

# CV model used
MODEL_FILEPATH = "yolo11s.pt"
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
    name="sig",
    cos_lr=False,
    cache=False,
    freeze=1,
    lr0=0.0033329,
    optimizer="SGD",
)

print(f"done, check {ROOT}")

