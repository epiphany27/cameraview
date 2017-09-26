/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.android.cameraview;

import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.media.Image;
import android.os.Build;
import android.support.annotation.RequiresApi;

import org.opencv.core.CvType;
import org.opencv.core.Mat;
import org.opencv.core.Size;

import java.nio.ByteBuffer;

/**
 * Created by pushkar on 26/09/17.
 */

public class ImageUtils {

    public static Mat bytesToMat(byte[] data, Camera camera) {

        Camera.Size previewFrameSize;

        try {
            previewFrameSize = camera.getParameters().getPreviewSize();
        } catch (Exception e) {
            e.printStackTrace();
            //Toast.makeText(this, "Failed to initialize camera", Toast.LENGTH_SHORT).show();
            return null;
        }
//        Log.d(TAG, "onPreviewFrame - received image " + previewFrameSize.width + "x" + previewFrameSize.height
//                + " focused: "+ mFocused +" imageprocessor: "+(imageProcessorBusy?"busy":"available"));
        //if ( mFocused && ! imageProcessorBusy ) {
          //  setImageProcessorBusy(true);
            Mat yuv = new Mat(new org.opencv.core.Size(previewFrameSize.width, previewFrameSize.height * 1.5), CvType.CV_8UC1);
            yuv.put(0, 0, data);

            Mat mat = new Mat(new Size(previewFrameSize.width, previewFrameSize.height), CvType.CV_8UC4);

        return mat;
    }

    /**
     * Takes an Android Image in the YUV_420_888 format and returns an OpenCV Mat.
     *
     * @param image Image in the YUV_420_888 format.
     * @return OpenCV Mat.
     */
    @RequiresApi(api = Build.VERSION_CODES.KITKAT)
    public static Mat imageToMat(Image image) {
        ByteBuffer buffer;
        int rowStride;
        int pixelStride;
        int width = image.getWidth();
        int height = image.getHeight();
        int offset = 0;

        Image.Plane[] planes = image.getPlanes();
        byte[] data = new byte[image.getWidth() * image.getHeight() * ImageFormat.getBitsPerPixel(ImageFormat.YUV_420_888) / 8];
        byte[] rowData = new byte[planes[0].getRowStride()];

        for (int i = 0; i < planes.length; i++) {
            buffer = planes[i].getBuffer();
            rowStride = planes[i].getRowStride();
            pixelStride = planes[i].getPixelStride();
            int w = (i == 0) ? width : width / 2;
            int h = (i == 0) ? height : height / 2;
            for (int row = 0; row < h; row++) {
                int bytesPerPixel = ImageFormat.getBitsPerPixel(ImageFormat.YUV_420_888) / 8;
                if (pixelStride == bytesPerPixel) {
                    int length = w * bytesPerPixel;
                    buffer.get(data, offset, length);

                    // Advance buffer the remainder of the row stride, unless on the last row.
                    // Otherwise, this will throw an IllegalArgumentException because the buffer
                    // doesn't include the last padding.
                    if (h - row != 1) {
                        buffer.position(buffer.position() + rowStride - length);
                    }
                    offset += length;
                } else {

                    // On the last row only read the width of the image minus the pixel stride
                    // plus one. Otherwise, this will throw a BufferUnderflowException because the
                    // buffer doesn't include the last padding.
                    if (h - row == 1) {
                        buffer.get(rowData, 0, width - pixelStride + 1);
                    } else {
                        buffer.get(rowData, 0, rowStride);
                    }

                    for (int col = 0; col < w; col++) {
                        data[offset++] = rowData[col * pixelStride];
                    }
                }
            }
        }

        // Finally, create the Mat.
        Mat mat = new Mat(height + height / 2, width, CvType.CV_8UC1);
        mat.put(0, 0, data);

        return mat;
    }
}
