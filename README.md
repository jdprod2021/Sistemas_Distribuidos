# Sistemas_Distribuidos
Simulador Blockchain

Código: 160004622
Nombre: Jose Daniel Barreto Aguilera

Requisitos 
- Sistema Linux.
- Tener compilador de C.

Pasos 
1. Descargar los archivos del repositorio
2. Abrir una terminal en el directorio Sistemas_Distribuidos
3. Escribir el comando:   make
4. Luego ejecutar el servidor, el cual cuenta con 4 parámetros, uno es el texto a usar, el cual puede modificar a gusto, luego tenemos 3 números:
   - Longitud de relleno
   - Dificultad (es decir cantidad de 0's al final que contiene el Hash para ser válido)
   - Workers
./servidor texto.txt [Long Relleno] [Dificultad] [Long. de Workers]
Un ejemplo sería

./servidor texto.txt 4 4 5

5. Ejecutar los worker
   En cada terminal se abre un worker 
./worker

NOTAS: 
El texto que se deja en texto.txt por defecto es sencillo, por tanto se puede modificar para que el intento de encontrar no se realice en el primer
rango de búsqueda y por tanto lo encuentre el primer worker

El servidor se sigue ejecutando así se haya encontrado la solución hasta que los 5 workers se hayan conectado.

