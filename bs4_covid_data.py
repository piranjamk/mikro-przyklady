import urllib.request
from bs4 import BeautifulSoup
import numpy as np
import matplotlib.pyplot as plt
from datetime import datetime
from datetime import datetime as dt
import os


def testowanie1():          # TEST REQUESTA NA BYLE CZYM
    pwsz_request = urllib.request.urlopen("http://www.pwsz.elblag.pl/")
    kod_strony = pwsz_request.read().decode()
    litery = 0
    liczby = 0
    samogloski = 0
    spolgloski = 0
    string_samoglosek = "aąeęiouy"
    #test = "/^)*($#@&$)(#"
    for i in kod_strony:
        if i.isdigit():
            liczby += 1
        if i.isalpha():
            litery += 1
            if string_samoglosek.find(i):
                samogloski += 1
            else:
                spolgloski += 1
    print("testowanie1")
    print("Liczby:     ", liczby)
    print("Litery:     ", litery)
    print("Samogłoski: ", samogloski)
    print("Spolgloski: ", spolgloski)
    act_dat = datetime.now()
    data = act_dat.strftime("%d/%m/%Y %H:%M:%S")
    print("Czas: ", data)
    print()
    stats_array_1 = [data, liczby, litery, samogloski, spolgloski]
    return stats_array_1


'''
statystyki COVID ze strony  + zapis do csv
'''


def testowanie_2():
    print("testowanie 2")
    req = urllib.request.Request("http://www.worldometers.info/coronavirus/", headers={'User-Agent': 'Mozilla/5.0'})
    strona = urllib.request.urlopen(req)
    kod_strony = strona
    soup = BeautifulSoup(kod_strony, "html.parser")
    dane = []

    # naglowki
    elements = soup.find('tr')
    naglowki = elements.find_all('th')
    print("Numery kolumn i ich wartosci:")
    tabela_naglowkow = [x.text.strip() for x in naglowki]
    dane.append(tabela_naglowkow)

    # wiersze tabeli
    zapis = []
    table_body = soup.find('tbody')
    rows = table_body.find_all('tr')
    for row in rows:
        cols = row.find_all('td')
        zapis.append([x.text.strip() for x in cols])
    dane.extend(zapis)

    # dane[0][13] = "Tests/ 1M pop"
    dane[0][13] = dane[0][13].replace('\n', '')  # zbedny escape sequence
    for numer, naglowek in enumerate(dane[0]):
        print("Numer:", numer, " ", naglowek)

    for i in range(len(dane)):
        for j in range(19):
            dane[i][j] = '0' if dane[i][j] == 'N/A' else dane[i][j]
            dane[i][j] = '0' if dane[i][j] == '' else dane[i][j]

    np.savetxt('corona.csv', dane[9:], fmt="%s", delimiter=';')
    return dane[0], dane[9:]


'''
wczytaj csv, posortuj, wyswietl
'''


def get_input_numbers(counted_countries):
    column_number_input = '1'
    country_number_input = ' '
    continue_loop = True
    try:
        while continue_loop:
            print("Podaj numer kolumny, wedlug ktorej posortowac dane (zakres 0 - 18 bez 1 i 15):")
            column_number_input = input()
            if not column_number_input.isdigit() or column_number_input == '1' \
                    or column_number_input == '15' \
                    or int(column_number_input) > 18:
                print("Podales niepoprawne dane! Sprobuj jeszcze raz")
            else:
                continue_loop = False
        continue_loop = True
        while continue_loop:
            print("Podaj liczbe krajow do wyswietlenia (zakres 0 - ", counted_countries, "):")

            country_number_input = input()
            if not country_number_input.isdigit() or int(country_number_input) > counted_countries:
                print("Podales niepoprawne dane lub zakres! Sprobuj jeszcze raz")
            else:
                continue_loop = False
    except ValueError:
        print("Podaj poprawne dane!")

    return int(column_number_input), int(country_number_input)


def process_and_sort(wczytane_dane, column_number):
    # konwersja danych tekstowych do liczbowych, usuwanie zbednych znakow
    for i in range(len(wczytane_dane)):
        for j in range(19):
            wczytane_dane[i][j] = wczytane_dane[i][j].replace(',', '')
            wczytane_dane[i][j] = wczytane_dane[i][j].replace('+', '')
            if wczytane_dane[i][j].isdigit():
                temp = int(wczytane_dane[i][j])
                wczytane_dane[i][j] = temp
    wczytane_dane[10][3] = wczytane_dane[10][3].replace(',', '')
    wczytane_dane[10][3] = wczytane_dane[10][3].replace('+', '')

    sorted_array = wczytane_dane[wczytane_dane[:, column_number].astype(float).argsort()]
    return sorted_array[::-1]


def plot_fig(sorted_array, country_number, column_number, headers_names):
    labels = np.asarray([sorted_array[:country_number, 1]])[0]
    values = (sorted_array[:country_number, column_number]).astype(float)
    widths = [0.8]
    for i in range(country_number - 1):
        widths.append(0.8)
    bottoms = np.asarray(np.zeros(country_number))

    plt.figure(figsize=(12, 6))
    fig = plt.subplot()
    plt.title("Wykres dla kolumny: " + headers_names[column_number])
    plt.bar(x=labels, height=values, width=widths, bottom=bottoms, align='center')
    plt.show()


def testowanie_3(headers_names):
    print()
    print("testowanie 3")
    wczytane_dane = np.loadtxt('corona.csv', dtype='str', delimiter=';')
    counted_countries = len(wczytane_dane)
    column_number, country_number = get_input_numbers(counted_countries)
    sorted_array = process_and_sort(wczytane_dane, column_number)
    plot_fig(sorted_array, country_number, column_number, headers_names)

'''
statsy do 1 i 2
'''


def testowanie_dodatkowe_1(stats_1, stats_2):
    print("testowanie dodatkowe 1")
    # test1
    if os.path.exists("plik1_stat.csv"):
        print("Plik ze statystykami znaleziony. Statystyki zostana dopisane.")
        data_from_file_1 = np.loadtxt("plik1_stat.csv", delimiter=';', dtype="str")
        lista1 = np.vstack((data_from_file_1, np.asarray(stats_1)))
        np.savetxt("plik1_stat.csv", lista1, delimiter=';', fmt="%s")
    else:
        print("Brak pliku. Plik zostanie stworzony. Zapisanie statystyk w nowym pliku")
        stats_1 = np.asarray(stats_1)
        np.savetxt("plik1_stat.csv", [stats_1], delimiter=';', fmt="%s")

    # test2
    act_dat = datetime.now()
    data = act_dat.strftime("%d/%m/%Y %H:%M:%S")
    for i in stats_2:
        i.append(data)
    if os.path.exists("plik2_stat.csv"):
        print("Plik ze statystykami znaleziony. Statystyki zostana dopisane.")
        data_from_file_2 = np.loadtxt("plik2_stat.csv", delimiter=';', dtype="str")
        lista2 = np.vstack((data_from_file_2, np.asarray(stats_2)))
        np.savetxt("plik2_stat.csv", lista2, delimiter=';', fmt="%s")
    else:
        print("Brak pliku. Plik zostanie stworzony. Zapisanie statystyk w nowym pliku")
        np.savetxt("plik2_stat.csv", stats_2, delimiter=';', fmt="%s")


'''
wykres do statsow
'''
def testowanie_dodatkowe_2(csv_headers):
    print("testowanie dodatkowe 2")
    print("Podaj rok od")
    rok_od = input()
    print("Podaj miesiac od")
    miesiac_od = input()
    print("Podaj dzien od")
    dzien_od = input()
    data_od = dt(int(rok_od), int(miesiac_od), int(dzien_od))
    print("Podaj rok do")
    rok_do = input()
    print("Podaj miesiac do")
    miesiac_do = input()
    print("Podaj dzien do")
    dzien_do = input()
    data_do = dt(int(rok_do), int(miesiac_do), int(dzien_do))


    # do test 1
    if not os.path.exists("plik1_stat.csv"):
        return
    plt.figure()
    fig1 = plt.subplot()
    stats_array = np.loadtxt("plik1_stat.csv", delimiter=';', dtype="str")
    x = []
    y_liczby = []
    y_litery = []
    y_samogloski = []
    y_spolgloski = []

    print()
    print("Dlugosc len:", len(stats_array))
    print("ndim", stats_array.ndim)
    if stats_array.ndim == 1:
        stats_array = [stats_array]
        line_pattern = 'D'
    else:
        line_pattern = ''
    for i in stats_array:
        i = np.asarray(i)
        if data_od <= dt.strptime(i[0][:10], '%d/%m/%Y') <= data_do:
            x.append(i[0])
            y_liczby.append(int(i[1]))
            y_litery.append(int(i[2]))
            y_samogloski.append(int(i[3]))
            y_spolgloski.append(int(i[4]))
    if len(x) > 0:
        if len(x) == 1:
            line_pattern = 'D'
        plt.plot(x, y_liczby, 'r' + line_pattern, label='liczby')
        plt.plot(x, y_litery, 'g' + line_pattern, label='litery')
        plt.plot(x, y_samogloski, 'b' + line_pattern, label='samogloski')
        plt.plot(x, y_spolgloski, 'y' + line_pattern, label='spolgloski')
        x_ticks = plt.MaxNLocator(3)
        fig1.xaxis.set_major_locator(x_ticks)
        plt.title("Wykres statystyk do testowania 1")
        plt.legend()
        plt.show()
    else:
        print("Brak danych w wybranym przedziale czasowym dla statystyk z testowania1")

    # do test 2
    if not os.path.exists("plik2_stat.csv"):
        return
    plt.figure()
    fig1 = plt.subplot()
    stats_array = np.loadtxt("plik2_stat.csv", delimiter=';', dtype="str")
    print(stats_array)

    country_name = 'Poland'
    column_number = '2'
    print("Podaj nazwe kraju (np. Poland, USA)")
    country_name = input()
    print("Podaj numer kolumny dla ktorej wyswietlic statystyki (0-18 bez 1 i 15)") #1 i 15 to nazwa kraju i kontynentu
    continue_loop = True
    while continue_loop:
        column_number = input()
        if not column_number.isdigit() or column_number == '1' or column_number == '15' or int(column_number) > 18:
            print("Podales niepoprawne dane! Sprobuj jeszcze raz")
        else:
            continue_loop = False

    x2 = [] # data
    y2 = [] # wartosc z wybranej kolumny
    for i in stats_array:
        i = np.asarray(i)
        if i[1] == country_name and data_od <= dt.strptime(i[19][:10], '%d/%m/%Y') <= data_do:
            x2.append(i[19])
            liczba = column_number.replace(',', '')
            liczba = int(liczba)

            wartosc = i[liczba].replace(',', '')
            wartosc = wartosc.replace('+', '')
            wartosc = wartosc.replace('N/A', '')

            y2.append(int(wartosc))
    if len(x) == 0:
        print("Nie znaleziono podanego kraju lub danych dla niego.")
    else:
        x_ticks = plt.MaxNLocator(3)
        fig1.xaxis.set_major_locator(x_ticks)
        string_title = "Historia statystyk zad2 - "+country_name+", kolumna: " + csv_headers[int(column_number)]
        plt.title(string_title)
        line_pattern = ''
        if len(x) == 1:
            line_pattern = 'D'
        plt.plot(x2, y2, 'g' + line_pattern)
        plt.show()


if __name__ == "__main__":
    actual_stats_array_1 = testowanie1()
    csv_headers, actual_stats_array_2 = testowanie_2()
    testowanie_3(csv_headers)
    testowanie_dodatkowe_1(actual_stats_array_1, actual_stats_array_2)
    testowanie_dodatkowe_2(csv_headers)

