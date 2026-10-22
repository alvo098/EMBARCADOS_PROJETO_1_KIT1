## Simulação no Tinkercad

Antes da montagem física, o projeto foi desenvolvido e testado no simulador **Tinkercad**. A simulação usa a primeira versão do código, com **sensor de presença PIR** e a biblioteca `Adafruit_LiquidCrystal`, que é a disponível no Tinkercad.

🔗 **[Abrir a simulação no Tinkercad](https://www.tinkercad.com/things/3LYaqM0YgVk-projetosentinelasimulacao?sharecode=YoyRjZ79GIJ0FnwO5tLPjFKNOMfsLPv99C_rSsuWmTs)**

### Como testar a simulação
1. Clique em **Iniciar simulação**.
2. Siga o tutorial no display: o botão **TELA** avança e o botão **MODO** sai.
3. Aperte **MODO** para armar e aguarde a contagem de 5 segundos.
4. Clique no sensor PIR e arraste o objeto para dentro da área de detecção para disparar o alarme.
5. Aperte **MODO** de novo para desarmar.

Também é possível controlar o sistema pelo **Monitor Serial**: `a` (armar), `d` (desarmar), `s` (status), `t` (tutorial) e `h` (ajuda).

### Diferenças para a versão física
Na montagem real, o PIR gerou disparos falsos, então foi substituído por um **sensor ultrassônico HC-SR04**. A biblioteca do display também mudou para `LiquidCrystal I2C`, que é compatível com o módulo I2C comum.
