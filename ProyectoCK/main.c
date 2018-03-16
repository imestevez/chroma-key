/*
 * * * * * * * * * * * * 
 * PROYECTO CHROMA KEY * 
 * * * * * * * * * * * * * * * * *
 * Autor: Iván Martínez Estévez  *
 * * * * * * * * * * * * * * * * *
 * * * * * * * * * * 
 * Grupo 6 *
 * * * * * *
 */

#include <stdio.h>
#include <stdlib.h>

#include <opencv/cv.h>
#include <opencv/highgui.h>

#include <math.h>
#include <time.h>
#include <Windows.h>

int main(int argc, char* argv[]) {

    //margen colores 
    int M = 90;
    int salir;


    //Abrimos el video
    CvCapture* capture = cvCaptureFromAVI("chromaExacto.avi");
    //Crea una imagen que será un frame del video
    IplImage* frame = cvQueryFrame(capture);

    //Imagen utilizada como fondo
    IplImage *imgFondo = cvLoadImage(argv[2], CV_LOAD_IMAGE_UNCHANGED);

    //Clona la imagen de fondo
    IplImage *imgClon = cvClone(imgFondo);

    //Iagen presentada del tamaño del video
    IplImage *imgPres = cvCreateImage(cvSize(frame->width, frame->height), IPL_DEPTH_8U, 3);


    if (!imgFondo && !frame) {
        printf("Error: ficheros %s no leidos\n", argv[1]);
        return EXIT_FAILURE;
    }

    // Fps = tiempo entre frames  
    int fps = (int) cvGetCaptureProperty(capture, CV_CAP_PROP_FPS);


    //Variables para recorrer las imagenes
    int fila, columna;

    //Bucle infinito
   

        //Para cada iteracion capturamos 1 frame
        frame = cvQueryFrame(capture);

        //Puntero a la fila 0 del frame 
        unsigned char *pimgFr = (unsigned char*) frame->imageData + 0 * frame->widthStep;

        //Mientras haya frames negros, capturamos el siguiente
        while (pimgFr[0] == 1 && pimgFr[1] == 1 && pimgFr[2] == 1) {
            frame = cvQueryFrame(capture);
        }
        //muestra el 1o frame no negro
        //cvShowImage("video", frame);
        //cvWaitKey(0);

        //Puntero al pixel (10,10) del frame
        unsigned char *pimgFrame = (unsigned char*) frame->imageData + 10 * frame->widthStep + (10 * frame->nChannels);

        //pimgFrame coje el valor de las componentes de color del pixel de muestra(10,10)
        int azul1 = *pimgFrame++;
        int verde1 = *pimgFrame++;
        int rojo1 = *pimgFrame++;

        
        struct timespec start, finish;
        double elapsed;
        clock_gettime(CLOCK_MONOTONIC, &start);
        do{
           
            //Recorre imagenes  
            for (fila = 0; fila < frame->height; fila++) {

                //Puntero a la imagen clonada
                unsigned char *pimgClon = (unsigned char*) imgClon->imageData + fila * imgClon->widthStep;
                //Puntero a la imagen final
                unsigned char *pimgPres = (unsigned char*) imgPres->imageData + fila * imgPres->widthStep;


                for (columna = 0; columna < frame->width; columna++) {


                    //puntero al píxel (fila,columna) del frame
                    unsigned char *pimgFrame = (unsigned char*) frame->imageData + fila * frame->widthStep + (columna * frame->nChannels);

                    //asignamos las CC del píxel apuntado por pimgFrame
                    int azul2 = *pimgFrame++;
                    int verde2 = *pimgFrame++;
                    int rojo2 = *pimgFrame++;

                    //Visualiza color chroma de muestra del pixel (10,10)
                    // printf("\t(chroma: %d, %d, %d)", azul1, verde1, rojo1);

                    //Visualiza color apuntado de cada píxel
                    // printf("\t(color .avi: %d, %d, %d)", azul2, verde2, rojo2);


                    if ((((azul1 - M) <= azul2) && (azul2 <= (azul1 + M)))
                            && (((verde1 - M) <= verde2) && (verde2 <= (verde1 + M)))
                            && (((rojo1 - M) <= rojo2) && (rojo2 <= (rojo1 + M)))
                            || ((verde2 < 180)&& (verde2 > 50)&&(azul2 < 6)&&(rojo2 < 6))
                            ) { //si se cumplen las condiciones

                        // Asignamos las CC de la imagen clonada a la imagen final y se incrementan
                        *pimgPres++ = *pimgClon++;
                        *pimgPres++ = *pimgClon++;
                        *pimgPres++ = *pimgClon++;

                        //incremento del puntero del frame
                        pimgFrame++;
                        pimgFrame++;
                        pimgFrame++;

                    } else { //si no se cumplen

                        // Asignación de las CC del frame a la imagen final y se incrementan
                        *pimgPres++ = *pimgFrame++;
                        *pimgPres++ = *pimgFrame++;
                        *pimgPres++ = *pimgFrame++;

                        //Incremento de la imagen clonada
                        pimgClon++;
                        pimgClon++;
                        pimgClon++;
                    }
                }
            }
            //Se muestra la imagen final
            cvShowImage("Presentacion", imgPres);
        


        //Capturamos tecla del teclado
        salir = cvWaitKey(1000 / fps);

        ///si se pulsa (ESC) sale del bucle
        if (salir == 27)break;
                    frame = cvQueryFrame(capture);

    } while(frame != 0) ;
    
             clock_gettime(CLOCK_MONOTONIC, &finish);
        elapsed = (finish.tv_sec - start.tv_sec);
        elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
        printf("Tiempo transcurrido: %f", elapsed);

    //libreamos direccion de memoria
    cvReleaseImage(&frame);
    cvReleaseImage(&imgFondo);
    cvReleaseImage(&imgClon);
    cvReleaseImage(&imgPres);

    //Cierra ventana de Windows
    cvDestroyWindow("Presentacion");


    return EXIT_SUCCESS;
}