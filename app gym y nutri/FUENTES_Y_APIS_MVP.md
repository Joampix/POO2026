# Conca Gym - Fuentes y APIs del MVP

## Objetivo del MVP

Aplicacion C++/Qt de escritorio enfocada en entrenamiento, alimentacion, progreso semanal y explicacion educativa para el usuario.

La app no depende 100% de internet: genera planes con logica local y usa APIs gratuitas como apoyo para ahorrar carga manual de datos.

## APIs gratuitas o abiertas

### Backend propio

- FastAPI propia
- Uso propuesto: login, autenticacion simple y registro de eventos importantes de la aplicacion.
- URL local: http://127.0.0.1:8000
- Persistencia del servidor: MySQL.
- Administracion visual de base de datos: phpMyAdmin.

### Base local

- SQLite local en Qt
- Uso propuesto: guardar intentos fallidos, ultimo login exitoso y eventos menores aunque el servidor no este disponible.

### Entrenamiento

- ExerciseDB OSS API
- Uso propuesto: biblioteca principal de ejercicios, musculos, equipamiento, instrucciones y GIFs.
- Ventaja: version gratuita sin API key, con aproximadamente 1.500 ejercicios en V1.
- URL: https://oss.exercisedb.dev/api/v1/
- Docs: https://oss.exercisedb.dev/docs

- wger API
- Uso propuesto: segunda fuente abierta para ejercicios, musculos, equipamiento, descripciones e imagenes.
- Ventaja: endpoints publicos de lectura sin autenticacion para ejercicios e ingredientes; util como respaldo/complemento de ExerciseDB.
- URL: https://wger.de/api/v2/
- Endpoints usados: `/exercise-translation/` y `/exerciseinfo/{id}/`
- Docs: https://wger.readthedocs.io/en/latest/api/api.html

### IA

- Gemini API
- Uso propuesto: Coach IA contextual para responder dudas sobre dieta, entrenamiento, progreso y adherencia.
- Ventaja: capa conversacional para el Coach IA; la app mantiene calculos de macros, recetas y rutina en C++.
- Docs: https://ai.google.dev/gemini-api/docs

## Fuentes oficiales para la seccion educativa

- Dietary Guidelines for Americans, USDA/HHS:
  https://www.fns.usda.gov/cnpp/dietary-guidelines-americans

- CDC Nutrition Guidelines and Recommendations:
  https://www.cdc.gov/nutrition/php/guidelines-recommendations/index.html

- CDC Nutrition Facts Label:
  https://www.cdc.gov/healthy-weight-growth/healthy-eating/nutrition-label.html

- NIH Office of Dietary Supplements:
  https://ods.od.nih.gov/factsheets/list-all/

## Criterio cientifico aplicado en la app

- La calculadora estima gasto energetico y macros a partir de peso, altura, edad, actividad y objetivo.
- El plan nutricional usa deficit moderado para perdida de grasa, superavit moderado para ganancia muscular y mantenimiento para recomposicion o salud general.
- La rutina se adapta al objetivo, dias disponibles, nivel y equipamiento.
- Cada ejercicio incluye una alternativa por si no hay maquina y consulta referencias en ExerciseDB + wger.
- Las recetas se generan con un motor local de ingredientes y macros para mantener el MVP estable.
- El progreso semanal registra peso, cintura, entrenamientos realizados, energia y notas.
- FastAPI registra login y eventos importantes en MySQL.
- SQLite local registra intentos y eventos menores para auditoria basica offline.

## Aviso necesario

La app debe presentarse como herramienta educativa. No reemplaza a un medico, nutricionista ni entrenador certificado.
