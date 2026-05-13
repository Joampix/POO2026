# Configurar Gemini en Conca Gym C++/Qt

La app usa Gemini API para el Coach IA. Si falta la clave, la seccion avisa `Gemini sin clave`.

## Configuracion recomendada

Ejecutar una sola vez desde la carpeta raiz del proyecto:

```powershell
.\configurar_gemini.ps1
```

O hacer doble click en `configurar_gemini.bat`.

Si copiaste la clave desde Google AI Studio, el script puede detectarla desde el portapapeles y guardarla sin mostrarla. Si ya existe una clave valida en `.env`, no vuelve a pedirla.

El script guarda la clave en:

- `.env`, archivo local ignorado por `.gitignore`.
- `GEMINI_API_KEY`, variable de entorno del usuario de Windows.

Despues la app la detecta automaticamente:

```powershell
.\run_cpp_qt.bat
```

Si antes aparecia `403 PERMISSION_DENIED`, vuelve a ejecutar `configurar_gemini.ps1` y pega la clave completa nuevamente. Ese error suele indicar clave invalida, incompleta, restringida o sin permisos para Gemini API.

Para reemplazar una clave guardada:

```powershell
.\configurar_gemini.ps1 -Forzar
```

Para guardar sin hacer prueba de conexion:

```powershell
.\configurar_gemini.ps1 -SinVerificar
```

## Formato del archivo .env

```text
GEMINI_API_KEY=TU_CLAVE
```

Tambien se acepta `GOOGLE_API_KEY`.

## Modelo usado

- `gemini-2.5-flash`
- Endpoint REST `generateContent`
- Documentacion oficial: https://ai.google.dev/gemini-api/docs

## Seguridad

La clave no va hardcodeada en C++ y `.env` no se sube. El Coach IA es una herramienta educativa: no reemplaza a un medico, nutricionista ni entrenador certificado.
