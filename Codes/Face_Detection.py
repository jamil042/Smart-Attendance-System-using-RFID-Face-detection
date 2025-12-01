import cv2
import numpy as np
import os
import urllib.request
import serial
import time
import pandas as pd
from datetime import datetime
from sklearn.metrics.pairwise import cosine_similarity


arduino = serial.Serial('COM3', 9600, timeout=1)  # Change COM port

# Attendance log file
excel_file = "attendance.xlsx"

# Load or create Excel
if not os.path.exists(excel_file):
    df = pd.DataFrame(columns=["ID", "Name", "Date", "Time"])
    df.to_excel(excel_file, index=False)


class FaceRecognitionAlternative:
    def __init__(self):
        self.face_cascade = cv2.CascadeClassifier(
            cv2.data.haarcascades + 'haarcascade_frontalface_default.xml'
        )
        self.face_recognizer = cv2.face.LBPHFaceRecognizer_create()
        self.known_names = []
        self.face_encodings = []

    def extract_face_features(self, face_img):
        gray = cv2.cvtColor(face_img, cv2.COLOR_BGR2GRAY)
        gray = cv2.resize(gray, (100, 100))
        gray = cv2.equalizeHist(gray)
        features = gray.flatten().astype(np.float32) / 255.0
        return features

    def load_known_faces(self, path):
        files = os.listdir(path)
        for i, filename in enumerate(files):
            if filename.lower().endswith(('.jpg','.png','.jpeg')):
                img = cv2.imread(os.path.join(path, filename))
                gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
                faces = self.face_cascade.detectMultiScale(gray, 1.1, 4)
                if len(faces) == 0:
                    continue
                x,y,w,h = faces[0]
                roi = img[y:y+h, x:x+w]
                feat = self.extract_face_features(roi)
                self.face_encodings.append(feat)
                self.known_names.append(os.path.splitext(filename)[0])
        return True

    def recognize_face(self, face_img):
        features = self.extract_face_features(face_img)
        sims = [cosine_similarity([features],[f])[0][0] for f in self.face_encodings]
        best_idx = int(np.argmax(sims))
        if sims[best_idx] > 0.6:
            return self.known_names[best_idx], sims[best_idx]
        return "Unknown", 0.0


# ================================
# MAIN LOOP: Wait RFID -> Face -> Excel
# ================================
def main():
    face_rec = FaceRecognitionAlternative()
    face_rec.load_known_faces("image_folder")

    esp32_url = "http://192.168.2.165/cam-hi.jpg"

    print("System Ready. Waiting for RFID...")

    while True:
        if arduino.in_waiting:
            line = arduino.readline().decode().strip()
            if line == "FACE_REQUEST":
                # Read additional info
                name, uid = "", ""
                while True:
                    l = arduino.readline().decode().strip()
                    if l.startswith("NAME:"): name = l[5:]
                    elif l.startswith("ID:"): uid = l[3:]
                    elif l == "END_REQUEST": break

                print(f"[RFID OK] Now verifying face for {name} ({uid})")

                # Face check (10 seconds window)
                verified = False
                start = time.time()
                while time.time() - start < 10:
                    try:
                        img_resp = urllib.request.urlopen(esp32_url, timeout=5)
                        img_np = np.array(bytearray(img_resp.read()), dtype=np.uint8)
                        frame = cv2.imdecode(img_np, -1)
                        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
                        faces = face_rec.face_cascade.detectMultiScale(gray, 1.1, 4)
                        for (x,y,w,h) in faces:
                            roi = frame[y:y+h, x:x+w]
                            who, conf = face_rec.recognize_face(roi)
                            if who == name and conf > 0.6:
                                print(f"✅ Face verified for {name}")
                                arduino.write(f"FACE_VERIFIED:{name}\n".encode())
                                log_attendance(uid, name)
                                verified = True
                                break
                        if verified: break
                    except Exception as e:
                        print("Camera error:", e)
                        continue

                if not verified:
                    print("❌ Face verification failed")
                    arduino.write(b"FACE_UNKNOWN\n")


def log_attendance(uid, name):
    now = datetime.now()
    df = pd.read_excel(excel_file)
    df.loc[len(df)] = [uid, name, now.strftime("%Y-%m-%d"), now.strftime("%H:%M:%S")]
    df.to_excel(excel_file, index=False)
    print("Attendance saved ✅")


if __name__ == "__main__":
    main()
