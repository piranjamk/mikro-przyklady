import numpy as np
import cv2 as cv
import time
import keyboard

'''             DEF                '''
KOLOR_MARTWY = np.array([0, 0, 0], np.uint8)
KOLOR_ZYWY = np.array([50, 150, 250], np.uint8)
WYSOKOSC = 100
SZEROKOSC = 100
plansza = np.full((WYSOKOSC, SZEROKOSC, 3), KOLOR_MARTWY)
BRUSHINGOWANIE = False  # malowanie zywych komorek
BRUSHOWANIE_DEAD = False  # malowanie martwych komorek
PRZYBLIZENIE = SZEROKOSC - 1
PRZESUN_HORYZONTALNIE = 0
PRZESUN_WERTYKALNIE = 0
'''             END - DEF          '''


def rysuj(event, x, y, flags, param):
    # if event == cv.EVENT_FLAG_LBUTTON:
    # if cv.waitKey(0) & 0xFF == 119:  # litera w
    global BRUSHINGOWANIE
    global plansza
    if event == cv.EVENT_LBUTTONDOWN:
        print("Mouse down!")
        BRUSHINGOWANIE = True
    elif event == cv.EVENT_LBUTTONUP:
        print("Mouse up!")
        BRUSHINGOWANIE = False
    if BRUSHINGOWANIE:
        plansza[y + PRZESUN_WERTYKALNIE, x + PRZESUN_HORYZONTALNIE] = KOLOR_ZYWY

    global BRUSHOWANIE_DEAD
    if event == cv.EVENT_RBUTTONDOWN:
        print("Mouse down!")
        BRUSHOWANIE_DEAD = True
    elif event == cv.EVENT_RBUTTONUP:
        print("Mouse up!")
        BRUSHOWANIE_DEAD = False
    if BRUSHOWANIE_DEAD:
        plansza[y + PRZESUN_WERTYKALNIE, x + PRZESUN_HORYZONTALNIE] = KOLOR_MARTWY
    print(y + PRZESUN_WERTYKALNIE, " ", x + PRZESUN_HORYZONTALNIE)


def sprawdz_liczbe_zywych_sasiadow(plansza, punkt_y, punkt_x):
    licznik = 0
    wartosci = [-1, 0, 1]
    for i in wartosci:
        x = punkt_x + i
        for j in wartosci:
            y = punkt_y + j
            if (y >= 0 and x >= 0) and (y < WYSOKOSC and x < SZEROKOSC):
                licznik += plansza[y, x][0]
    return licznik / 50
    # print("Drukuje:", y, " ", x)


def zmien_stany(plansza):
    plansza_temp = np.full((WYSOKOSC, SZEROKOSC, 3), KOLOR_MARTWY)
    for h in range(WYSOKOSC):
        for w in range(SZEROKOSC):
            stan = "martwy"
            if plansza[h, w][0] != 0:
                stan = "zywy"
            ilosc_zywych_sasiadow = sprawdz_liczbe_zywych_sasiadow(plansza, h, w)
            if stan == "zywy":
                if ilosc_zywych_sasiadow < 2 or ilosc_zywych_sasiadow > 3:
                    plansza_temp[h, w] = KOLOR_MARTWY
                else:
                    plansza_temp[h, w] = KOLOR_ZYWY
            else:
                if ilosc_zywych_sasiadow == 2 or ilosc_zywych_sasiadow == 3:
                    plansza_temp[h, w] = plansza_temp[h, w] = KOLOR_ZYWY
    return plansza_temp


def scrolluj(v):
    global PRZYBLIZENIE
    global PRZESUN_HORYZONTALNIE
    PRZYBLIZENIE = SZEROKOSC - v
    if PRZYBLIZENIE == 0:
        PRZYBLIZENIE = 1
    if PRZYBLIZENIE == SZEROKOSC:
        PRZYBLIZENIE = PRZYBLIZENIE - 1
        cv.setTrackbarPos('Delta x', 'image', 0)
        cv.setTrackbarPos('Delta y', 'image', 0)
    przesun_horyzontalnie(cv.getTrackbarPos('Delta y', 'image'))
    przesun_wertykalnie(cv.getTrackbarPos('Delta y', 'image'))


def przesun_horyzontalnie(v):
    global PRZESUN_HORYZONTALNIE
    global SZEROKOSC
    PRZESUN_HORYZONTALNIE = int((SZEROKOSC - 1 - PRZYBLIZENIE) / 99 * v)
    print(PRZESUN_HORYZONTALNIE)


def przesun_wertykalnie(v):
    global PRZESUN_WERTYKALNIE
    global WYSOKOSC
    PRZESUN_WERTYKALNIE = int((WYSOKOSC - 1 - PRZYBLIZENIE) / 99 * v)
    print(PRZESUN_WERTYKALNIE)


for i in range(2):
    plansza[0, i] = KOLOR_ZYWY
for i in range(WYSOKOSC):
    plansza[i, int(SZEROKOSC / 2) - 1] = KOLOR_ZYWY
    plansza[i, int(SZEROKOSC / 2) - 2] = KOLOR_ZYWY
    plansza[i, int(SZEROKOSC / 2) - 3] = KOLOR_ZYWY

plansza[0:WYSOKOSC - 1, SZEROKOSC - 1] = KOLOR_ZYWY
cv.namedWindow('image', cv.WINDOW_NORMAL)
cv.setMouseCallback('image', rysuj)  # callback mysz

cv.createTrackbar('Scroll', 'image', 0, SZEROKOSC, scrolluj)
cv.createTrackbar('Delta x', 'image', 0, SZEROKOSC, przesun_horyzontalnie)
cv.createTrackbar('Delta y', 'image', 0, WYSOKOSC, przesun_wertykalnie)

video = cv.VideoWriter("video.avi", cv.VideoWriter_fourcc(*"MJPG"), 5, (SZEROKOSC, WYSOKOSC))

numer_iteracji = 0
ODTWARZAJ = False


while 1:
    if cv.waitKey(1) & 0xFF == 32:  # press 'space' to pause/auto-play
        if ODTWARZAJ:
            ODTWARZAJ = False
        else:
            ODTWARZAJ = True
        spowolnienie = 1
        print("Auto odtwarzanie:", ODTWARZAJ)
        time.sleep(2)
    if ODTWARZAJ:
        plansza = np.copy(zmien_stany(plansza))
        print(numer_iteracji)
        numer_iteracji += 1

    if cv.waitKey(1) & 0xFF == 97:  # press 'a' to manually play
        plansza = np.copy(zmien_stany(plansza))
        print(numer_iteracji)
        numer_iteracji += 1
        time.sleep(1)
    # przewijanie
    y1 = 0 + PRZESUN_WERTYKALNIE
    y2 = PRZYBLIZENIE + PRZESUN_WERTYKALNIE
    x1 = 0 + PRZESUN_HORYZONTALNIE
    x2 = PRZYBLIZENIE + PRZESUN_HORYZONTALNIE
    if x2 > 99:
        x2 = 99
    # przewijanie - end
    cv.imshow('image', plansza[y1:y2, x1: x2])  # wyswietlanie + przewijanie
    video.write(plansza)

    if cv.waitKey(1) & 0xFF == 27:  # press 'esc' to exit
        break

cv.destroyAllWindows()
video.release()
