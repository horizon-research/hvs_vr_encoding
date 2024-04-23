import cv2
import argparse
import os
from tqdm import tqdm

def encode_images_to_video(images_folder, video_path, fps):
    images = [img for img in os.listdir(images_folder) if img.endswith(".jpg")]
    file_num = len(images)

    total_frames = file_num

    # get shape from first frame
    frame = cv2.imread(os.path.join(images_folder, images[0]))
    height, width, layers = frame.shape
    size = (width, height)

    out = cv2.VideoWriter(video_path, cv2.VideoWriter_fourcc(*'mp4v'), fps, size)

    with tqdm(total=total_frames, desc="Encoding Video") as pbar: 
        for i in range(file_num):
            img_filename = images_folder + "/" + "frame" + str(i) + ".jpg"
            frame = cv2.imread(img_filename)
            out.write(frame)
            pbar.update(1)

    out.release()
    cv2.destroyAllWindows()


if __name__ == '__main__':

    parser = argparse.ArgumentParser()
    parser.add_argument('--fps', type=int, help='frames per second')
    parser.add_argument('--images_folder', type=str, help='path to the images folder')
    parser.add_argument('--video_path', type=str, help='path to the output video')
    args = parser.parse_args()

    encode_images_to_video(args.images_folder, args.video_path, args.fps)
