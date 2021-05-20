import urllib.request
from bs4 import BeautifulSoup
import numpy as np

IGNORED_EXTENSIONS = [
    # images
    'mng', 'pct', 'bmp', 'gif', 'jpg', 'jpeg', 'png', 'pst', 'psp', 'tif',
    'tiff', 'ai', 'drw', 'dxf', 'eps', 'ps', 'svg',
    # audio
    'mp3', 'wma', 'ogg', 'wav', 'ra', 'aac', 'mid', 'au', 'aiff',
    # video
    '3gp', 'asf', 'asx', 'avi', 'mov', 'mp4', 'mpg', 'qt', 'rm', 'swf', 'wmv',
    'm4a',
    # other
    'css', 'pdf', 'doc', 'exe', 'bin', 'rss', 'zip', 'rar',
]  # SO - https://stackoverflow.com/questions/12140460/how-to-skip-some-file-type-while-crawling-with-scrapy


def pobierz_wszystkie_linki_z_akutalnej_strony(adres_url):
    linki = []
    status = -1
    try:
        request = urllib.request.Request(adres_url)

        http_resp = urllib.request.urlopen(request)
        tresc_strony = BeautifulSoup(http_resp, 'html.parser')
        for link in tresc_strony.findAll('a'):
            if type(link.get('href')) == str:
                linki.append(link.get('href'))
        status = 0
    except:
        pass  # bledny adres, brak zasobu itd.
    return status, linki


def join_linki(linki):
    spreparowane_linki = []
    for i in linki:
        if i.find("//opegieka.pl") > -1:
            spreparowane_linki.append(i)
    return spreparowane_linki
'''
        elif (i[-1:].find('/') > -1  # ad2
              or i[-4:].find('html') > -1 or i[-2:].find('pl') > -1 or i[-3:].find('com') > -1) and i[:4].find(
            'http') == -1:  # ad3
            # print("dodany:  ", "https://pwsz.elblag.pl/"+i)
            # wykluczenie
            spreparowane_linki.append("https://opegieka.pl/" + i)
            for i in spreparowane_linki:
                print(i)
    return spreparowane_linki
'''


def przefiltruj_linki(linki):
    przefiltrowane_linki = []
    for i in linki:
        pass_iteration = False
        for x in IGNORED_EXTENSIONS:
            wartosc = i.rfind(x)
            if i.rfind(x) == len(i) - 3:
                pass_iteration = True
        if i.find('mailto') > -1:
            pass_iteration = True
        if (pass_iteration == True):
            continue
        if i.find('#') > -1:
            i = (i[0: i.find('#')])
        przefiltrowane_linki.append(i)
    return przefiltrowane_linki


def modyfikuj_liste_wystapien(linki, strony_do_odwiedzenia, odwiedzone_strony, adres_url, ilosc_odwiedzin):
    for i in linki:
        if (i not in odwiedzone_strony) and (i not in strony_do_odwiedzenia):
            strony_do_odwiedzenia[i] = 1
        elif (i not in odwiedzone_strony):
            temp = strony_do_odwiedzenia[i]
            temp += 1
            strony_do_odwiedzenia.update({i: temp})
            strony_do_odwiedzenia.update({i: temp})
        else:  # strona byla juz odwiedzona
            temp = odwiedzone_strony[i]
            temp += 1
            odwiedzone_strony.update({i: temp})


def przeszukaj_domene():
    print()
    odwiedzone_strony = {}
    strony_do_odwiedzenia = {'https://opegieka.pl/': 1}
    counter_sprawdzonych_zasobow = 0
    while (not strony_do_odwiedzenia) == False:
        counter_sprawdzonych_zasobow += 1
        print("Licznik sprawdzonych zasobow: ", counter_sprawdzonych_zasobow)
        adres_url, ilosc_odwiedzin = strony_do_odwiedzenia.popitem()
        odwiedzone_strony[adres_url] = ilosc_odwiedzin

        status_powodzenia, brudne_linki = pobierz_wszystkie_linki_z_akutalnej_strony(adres_url)
        linki_zjoinowane = join_linki(brudne_linki)  # laczenie niepelnych linkow -> np.  https://strona.pl + /podstrona
        linki_przefiltrowane = przefiltruj_linki(linki_zjoinowane)  # usuwanie niepozadanych adresow i rozszerzn np jpg, wmv
        if (status_powodzenia == 0):
            odwiedzone_strony.update({adres_url: ilosc_odwiedzin})
        modyfikuj_liste_wystapien(linki_przefiltrowane, strony_do_odwiedzenia, odwiedzone_strony, adres_url, ilosc_odwiedzin)
    odwiedzone_sorted = sorted(odwiedzone_strony.items(), reverse=True, key=lambda x: x[1])  # po wartosciach
    return odwiedzone_sorted


def stworz_prostego_page_ranka(lista_stron):
    print("Tworzenie page rank, prosze czekac...")
    odwiedzone_strony = {}
    strony_do_odwiedzenia = {'https://opegieka.pl/': 1}

    macierz_linkow = np.zeros((len(lista_stron), len(lista_stron)))
    while (not strony_do_odwiedzenia) == False:
        adres_url, ilosc_odwiedzin = strony_do_odwiedzenia.popitem()
        odwiedzone_strony[adres_url] = ilosc_odwiedzin

        status_powodzenia, brudne_linki = pobierz_wszystkie_linki_z_akutalnej_strony(adres_url)
        linki_zjoinowane = join_linki(brudne_linki)  # laczenie niepelnych linkow -> np.  https://strona.pl + /podstrona
        linki_przefiltrowane = przefiltruj_linki(
            linki_zjoinowane)  # usuwanie niepozadanych adresow i rozszerzn np jpg, wmv
        if (status_powodzenia == 0):
            odwiedzone_strony.update({adres_url: ilosc_odwiedzin})

        modyfikuj_liste_wystapien(linki_przefiltrowane, strony_do_odwiedzenia, odwiedzone_strony, adres_url,
                                  ilosc_odwiedzin)

        # kod dla wlasciwego tworzenia page ranka
        y = lista_stron.index(adres_url)
        for i in linki_przefiltrowane:
            x = lista_stron.index(i)
            # z obecnej strony do innej moze byc wiecej niz 1 link
            # uwzglednienie linkowania strony do samej siebie
            macierz_linkow[x, y] = macierz_linkow[x, y] + (1 / len(linki_przefiltrowane))
    licz_iteracyjnie = True
    iteracja = 0
    # zaczynamy od wartosci domyslnej = 0.25 dla kazdej ze stron
    print("Poczatkowa wartosc page rank dla kazdej strony: ", 1 / len(lista_stron))
    biezacy_macierz_rankow = np.full((len(lista_stron), 1),[1 / len(lista_stron)])
    poprzedni_macierz_rankow = np.copy(biezacy_macierz_rankow)
    while licz_iteracyjnie==True:
        iteracja += 1
        biezacy_macierz_rankow = np.matmul(macierz_linkow, biezacy_macierz_rankow)
        suma_rankow_w_iteracji = (biezacy_macierz_rankow.sum(axis=0))[0]
        delta_check = True
        for i in range(len(biezacy_macierz_rankow)):
            if abs(biezacy_macierz_rankow[i] - poprzedni_macierz_rankow[i]) > 0.01:
                delta_check = False
                break
        if (iteracja == 100) or (delta_check == True):
            licz_iteracyjnie = False
        poprzedni_macierz_rankow = np.copy(biezacy_macierz_rankow)  
    print("Wykonano ", iteracja, " iteracji.")
    rank_dictionary = {}
    for i in range(len(lista_stron)):
        rank_dictionary.update({lista_stron[i] : biezacy_macierz_rankow[i, 0]})
    page_rank_sorted = sorted(rank_dictionary.items(), reverse=True, key=lambda w: w[1])  # po wartosciach
    return page_rank_sorted

#przeszukiwanie domeny
slownik_stron_i_odwiedzin_sorted = przeszukaj_domene()
print("Wyniki przeszukiwania (adres url, liczba ")
#for i in slownik_stron_i_odwiedzin_sorted:
#    print(i)
print("Podaj ilosc najczesciej wystepujacych wynikow do wyswietlenia:")
ilosc = int(input())
licznik = 0
for i in slownik_stron_i_odwiedzin_sorted:
    licznik += 1
    if licznik > ilosc:
        break
    print(i)
#tworzenie page rank
lista_stron = []
for i in slownik_stron_i_odwiedzin_sorted:
    lista_stron.append(i[0])
print()
page_rank = stworz_prostego_page_ranka(lista_stron)
#print("Strony posortowane wedlug page rank:")
#for i in page_rank:
#    print(i)
print("Podaj ilosc elementow do wydruku dla page ranka:")
ilosc = int(input())
licznik = 0
for i in page_rank:
    licznik += 1
    if licznik > ilosc:
        break
    print(i)
np.savetxt("przeszukane.csv", slownik_stron_i_odwiedzin_sorted, fmt='%s')
np.savetxt("pagerank.csv", page_rank, fmt='%s')


