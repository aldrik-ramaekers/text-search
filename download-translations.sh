#!/bin/bash

# download translations
# https://poeditor.com/projects/

# MO FILE FORMAT: data/translations/[id]-[Display name].mo

###### ENGLISH
code=$(curl -X POST https://poeditor.com/api/ \
     -d api_token="89856d745c143ad91aca0f91c9e7640b" \
     -d action="export" \
     -d id="267905" \
     -d language="en" \
     -d type="mo" | jq -r '.item')

wget -O data/translations/en-English.mo "$code"
wget -O data/imgs/en.png https://www.countryflags.io/gb/shiny/24.png


##### DUTCH
code=$(curl -X POST https://poeditor.com/api/ \
     -d api_token="89856d745c143ad91aca0f91c9e7640b" \
     -d action="export" \
     -d id="267905" \
     -d language="nl" \
     -d type="mo" | jq -r '.item')

wget -O data/translations/nl-Dutch.mo "$code"
wget -O data/imgs/nl.png https://www.countryflags.io/nl/shiny/24.png
