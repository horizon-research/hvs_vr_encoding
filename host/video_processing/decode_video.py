import cv2
import argparse
import os
from tqdm import tqdm

def change_video_to_image(video_path, image_path):
    cap = cv2.VideoCapture(video_path)
    total_frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))

    count = 0
    with tqdm(total=total_frames, desc="Converting Video") as pbar:
        while cap.isOpened():
            ret, frame = cap.read()
            if ret:
                cv2.imwrite(os.path.join(image_path, f"frame{count}.jpg"), frame)
                count += 1
                pbar.update(1)
            else:
                break

    cap.release()
    cv2.destroyAllWindows()


if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    parser.add_argument('--video_path', type=str, help='path to the 360-video')
    parser.add_argument('--out_images_folder', type=str, help='path to the output video')
    args = parser.parse_args()

    # step 1: make a dir for output image
    if not os.path.exists(args.out_images_folder):
        os.makedirs(args.out_images_folder)

    change_video_to_image(args.video_path, args.out_images_folder)
    