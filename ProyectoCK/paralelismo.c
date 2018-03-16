/*
 * * * * * * * * * * * * 
 * PROYECTO CHROMA KEY * 
 * * * * * * * * * * * * * * * * *
 * Autor: Iván Martínez Estévez  *
 * * * * * * * * * * * * * * * * *
 * DNI: 44488293-Z *
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

#include <xmmintrin.h> 
#include  <emmintrin.h>

#define NTHREADS 2

IplImage *frame;
IplImage *frame4c;
IplImage *imgFondo;
IplImage *imgClon;
IplImage *imgFondo4c;
IplImage *imgPres;

int fps;
int salir;

__m128i CC_Frame, CC_Fondo, Verde, ImprimeFondo, ImprimeFrame, Total, V_Chroma;

void CK_threads(void *ptr) {
    int *p_Entero = (int *) ptr;
    int fila, cc;

    for (fila = *p_Entero; fila < imgPres->height; fila += NTHREADS) {

        //Puntero a la imagen clonada
        __m128i *pimgClon = (__m128i*) (imgClon->imageData + fila * imgClon->widthStep);
        //Puntero a la imagen final
        __m128i *pimgPres = (__m128i*) (imgPres->imageData + fila * imgPres->widthStep);


        for (cc = 0; cc < (imgPres->width * imgPres->nChannels); cc += 16) {


            //puntero al píxel (fila,cc) del frame
            __m128i *pframe = (__m128i*) (frame4c->imageData + fila * frame4c->widthStep + cc);

            //asignamos las CC del píxel apuntado por el frame
            CC_Frame = _mm_loadu_si128(pframe);
            //asignamos las CC del píxel apuntado por el fondo
            CC_Fondo = _mm_loadu_si128(pimgClon);
            //Comparamos el Verde Chroma con el color actual del video
            Verde = _mm_cmpeq_epi16(V_Chroma, CC_Frame);
            
            //Se queda con las componentes del fondo a imprimir
            ImprimeFondo = _mm_and_si128(Verde, CC_Fondo);
            //se queda con las componentes del frame a imprimir (distinto de verde)
            ImprimeFrame = _mm_andnot_si128(Verde, CC_Frame);
            
            // suma las componentes a imprimir
            Total = _mm_add_epi32(ImprimeFondo, ImprimeFrame);
                
            //almacena el valor en la imagen a presentar
            _mm_store_si128(pimgPres, Total);
               
            //incrementa punteros
            pimgPres++;
            pimgClon++;
            pframe++;

        }
    }
}
//Transforma el fondo en imagen de 4 canales
void img4canales() {

    int fila, columna;
    for (fila = 0; fila < imgFondo->height; fila++) {

        //Puntero al fondo
        unsigned char *pImgFondo = (unsigned char*) imgFondo->imageData + fila * imgFondo->widthStep;

        //Puntero a la imagen final
        unsigned char *pImgFondo4c = (unsigned char*) imgFondo4c->imageData + fila * imgFondo4c->widthStep;



        for (columna = 0; columna < imgFondo->width; columna++) {

            *pImgFondo4c++ = *pImgFondo++;
            *pImgFondo4c++ = *pImgFondo++;
            *pImgFondo4c++ = *pImgFondo++;
            *pImgFondo4c++ = 0;


        }
    }
}
//transforma cada frame a una imagen de 4 canales
void frame4canales() {

    int fila, columna;

    for (fila = 0; fila < frame->height; fila++) {

        unsigned char *pframe = (unsigned char*) frame->imageData + fila * frame->widthStep;
        unsigned char *pframe4c = (unsigned char*) frame4c->imageData + fila * frame4c->widthStep;

        for (columna = 0; columna < frame->width; columna++) {

            *pframe4c++ = *pframe++;
            *pframe4c++ = *pframe++;
            *pframe4c++ = *pframe++;
            *pframe4c++ = 0;

        }
    }
}

int main(int argc, char* argv[]) {


    //Abrimos el video
    CvCapture* capture = cvCaptureFromAVI("chromaExacto.avi");
    //Crea una imagen que será un frame del video
    frame = cvQueryFrame(capture);

    //Imagen utilizada como fondo
    imgFondo = cvLoadImage(argv[2], CV_LOAD_IMAGE_UNCHANGED);
    //imagen de 4 canales
    imgFondo4c = cvCreateImage(cvSize(imgFondo->width, imgFondo->height), IPL_DEPTH_8U, 4);
    //frame de 4 canales
    frame4c = cvCreateImage(cvSize(frame->width, frame->height), IPL_DEPTH_8U, 4);
   
    //crea imagen fondeo de 4 canales
    img4canales();


    //Clona la imagen de fondo 4 canales
    imgClon = cvClone(imgFondo4c);


    //Iagen presentada del tamaño del video
    imgPres = cvCreateImage(cvSize(frame4c->width, frame4c->height), IPL_DEPTH_8U, 4);


    if (!imgFondo && !frame) {
        printf("Error: ficheros %s no leidos\n", argv[1]);
        return EXIT_FAILURE;
    }

    // Fps = tiempo entre frames  
    int fps = (int) cvGetCaptureProperty(capture, CV_CAP_PROP_FPS);

    //captura 1 frame y lo convierte a 4 canales
    frame = cvQueryFrame(capture);
    frame4canales();

    //puntero al pixel (0,0) del frame
    __m128i *pimgFr = (__m128i*) frame4c->imageData;
    // almacena el verde chroma
    V_Chroma = _mm_loadu_si128(pimgFr);

//
//        struct timespec start, finish;
//        double elapsed;
//        clock_gettime(CLOCK_MONOTONIC, &start);

    do {


        //los threads se reparten por filas de cada frame
        pthread_t threads[NTHREADS];
        int filas[NTHREADS];
        int i;
        
        for (i = 0; i < NTHREADS; i++) {
            filas [i] = i;
            // printf("\nEl thread %d sustituye las filas %d", i, filas[i]);
            pthread_create(&threads[i], NULL, (void *) &CK_threads, (void *) &filas[i]);
        }
        for (i = 0; i < NTHREADS; i++) {
            pthread_join(threads[i], NULL);
        }

       
        // muestra la presentacion final
        cvShowImage("Presentacion", imgPres);
        salir = cvWaitKey(1000/fps);
        //si se pulsa (ESC) sale del bucle
        if (salir == 27)break;
        
        //captura un nuevo frame y lo convierte a 4 canales
        frame = cvQueryFrame(capture);
        frame4canales();


    } while (frame != 0);
    
//      clock_gettime(CLOCK_MONOTONIC, &finish);
//        elapsed = (finish.tv_sec - start.tv_sec);
//        elapsed += (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
//        printf("Tiempo transcurrido: %f", elapsed);

    //libreamos direccion de memoria
    cvReleaseImage(&frame);
    cvReleaseImage(&frame4c);
    cvReleaseImage(&imgFondo);
    cvReleaseImage(&imgFondo4c);
    cvReleaseImage(&imgClon);
    cvReleaseImage(&imgPres);

    //Cierra ventana de Windows
    cvDestroyWindow("Presentacion");


    return EXIT_SUCCESS;
}