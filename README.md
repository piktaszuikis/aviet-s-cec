Programa įjungs televizorių ir pasirinks save kaip įvesties šaltinį (TV pradės rodyti avietę). Tada programa pradės klausytis pultelių mygtukų ir bandys juos išversti į klaviatūros mygtukus.

Kokie pultelio mygtukai į kokius klaviatūros mygtukus išverčiami aprašyta kintamajame **pultelio_veiksmai**.

Daugiau apie projektą: http://piktas-zuikis.netlify.com/2020/05/24/TV-HDMI-RPI-CEC/

# Kompiliavimas
Reikalingi pakeitai:
```
pacman -S libcec xdotool
```

Kompiliavimas:
```
git clone https://github.com/piktaszuikis/avietės-cec.git
mkdir avietės-cec/build
cd avietės-cec/build
cmake ../
make
```

Viskas!

# Paleidimas
```
./aviete-cec
```

