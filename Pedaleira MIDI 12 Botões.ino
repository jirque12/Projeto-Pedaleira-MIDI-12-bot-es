#include <MIDI.h>
#define Afinador 0
#define PC_Message 1
#define CC_Message 2
#define Snapshot 3

unsigned long botaoPressUlt = 0; //Timer que guarda a ultima vez que o botão foi pressionado.
const long timerBotao = 5; /*Esse número é o tempo (em milisegundos) que o código espera até poder disparar outra mensagem MIDI.
Ele serve para que o botão não envie várias vezes a mesma mensagem ao pressionar o botão apenas uma vez. Se caso ainda acontecer
do botão mandar várias mensagens, aumente esse número até que ele mande apenas uma mensagem por vez.*/
const byte qntBotoes = 12; //Quantidade de Botões
const byte pinoBotoes[qntBotoes] = {00, 01, 02, 03, 04, 05, 06, 08, 09, 10, 11}; /*Aqui, definimos em quais pinos digitais estarão nossos botões.
Podemos escolher qualquer pino. E para facilitar, melhor que seja em sequência. No meu caso, usei duas portas fora da sequencia, pois as portas
25 e 29 não estavam funcionando*/
byte comandoMidi[qntBotoes] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}; /*Aqui, definimos qual o comando midi será enviado através de cada botão.
Aqui, vale ressaltar que essa matriz está em ordem crescente. Então o botão 1(que está na porta 22) vai mandar o comando 0, bem como o botão 4,
(que está na porta 53), vai mandar o comando 3. Esses números podem ser alterados para QUALQUER número, desde que respeite o limite MIDI, que é
0 a 127.*/
byte snapshot[qntBotoes] {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};/*Aqui, definimos qual o número do Snapshot que vamos acessar à partir de cada botão.
No meu caso, eu uso os 5 primeiros botões para Snapshot, logo, de 0 a 4.*/
byte travaBotao[qntBotoes] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; //Leia embaixo
byte ultimaTravaBotao[qntBotoes] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; /*Essas duas váriaveis são responsáveis por "travar o botão", prevenindo
mensagens MIDI de serem enviadas continuamente. Com elas implementadas, as mensagens MIDI são enviadas somente ao pressionar o botão.*/
byte estadoBotao[qntBotoes] {2, 2, 2, 2, 2, 2, 2, 2, 2, 2}; /*Essa variável, como o nome sugere, define o estado do botão. É aqui que o
meu código fica bem diferente de todos os outros que eu já havia visto. Isso porquê, comprei chaves de pedal (DPDT/3PDT), e as usadas nos outros
controladores eram chaves de "pulso", sem trava.
Então, nessa parte do código eu digo que o estado do botão é 2. Usei o 2, pois mais para frente no código, utilizo as os estados 0 e 1, para ligar
e desligar. Sendo o estado 2, ele não vai enviar o comando MIDI enquanto o código é incializado. Isto é bom, pois se no meio de uma música o
controlador desligar por qualquer motivo, quando ele for ligado novamente, não enviará nenhum comando indesejado.
Após iniciado o código, o estado do botão será definido de acordo com a matriz "definirEstadoBotao". Isso dá uma flexibilidade maior ao botão.
Por exemplo: No preset 1, o botão 1 deve estar ligado, porém no Preset 2, o mesmo botão deve estar desligado. Isso é facilmente definido através
da matriz "definirEstadoBotao". Lembrando que é necessário que o estado na matriz deve ser o mesmo do efeito na pedaleira/VST, do contrário será
necessário pisar no mesmo botão duas vezes para que o efeito mude.*/

byte estadoLed[2][qntBotoes] = {32, 33, 34, 35, 36, 37, 38, 39, 40, 41,42,43},//Matriz onde identificamos os LED's Vermelhos.
/*Eu utilizei LED's RGB para meu controlador, porém é possivel fazer com led de uma cor apenas, ou simplesmente não utilizar os LED's.
Porém, eu acredito que os LED's sejam cruciais, para um recurso visual.
No meu caso, utilizo o LED vermelho para indicar Stompbox ou Snapshots, e o LED azul para presets e afinador, mas isso pode ser mudado.
No caso do LED RGB, é possível também ativar duas ou três cores ao mesmo tempo, fazendo assim novas cores e aumentando a possibilidade
de recursos visuais.*/ 

byte funcao[qntBotoes] = {3, 3, 3, 3, 3, 3, 3, 3, 3, 3}; /*Aqui, nós definimos a função de cada botão, de acordo com o número definido para
cada função, lá em cima em #define. Nesse exemplo, os quatro primeiros são Stompobox (liga/desliga), o 5, 6 e 7 são para mudança de presets,
8 e 9 para Snapshots e 10 para afinador. Note que em alguns casos não é necessário fazer uma função separada para o afinador, porém eu utilizo
uma função separada, pois em alguns plugins, é necessário mandar mensagens MIDI em canais diferentes para que elas não acionem efeitos indesejados.*/

byte definirEstadoBotao[qntBotoes][qntBotoes] = /*Aqui temos uma matriz dentro de uma matriz. Parece complicado, mas não é muito.
Eu idealizei essa função para que assim que eu pressionasse o botão configurado como Preset, ele acenderia os LED's dos efeitos que estão ligados
na pedaleira/VST. 
Ex.: Vamos supor que eu tenho um Overdrive e um Delay no preset. Ambos estão na posição 0 e posição 1. No preset 1, ambos estão ligados. No preset 2,
ambos estão desligados.
Ao acionar o preset 1, os leds de ambos os botões configurados para esses efeitos, serão ligados (consequentemente, o código irá entender que eles estão
ligados, e assim que pressionados, eles desligarão o LED e o efeito). Quando o preset 2 for acionado, acontecerá o oposto.
Cada coluna (a coluna começa em { e termina em }, representa UM dos botões. E cada um dos números dentro dele, representa os leds, em ordem
crescente.
0 = Desligado;
1 = Ligado;
Sendo assim, se a coluna 1 for: {1,1,1,1,1,1,1,1,1,1}, TODOS os leds serão desligados (exceto o led do botão 1, que nos indicar que aquele é o preset sendo
usado atualmente).
Já se a coluna 4 estiver configurada {0,1,1,1,1,0,1,1,1,0}, os LED's 1, 5 e 10, serão ligados (bem como o LED 4, que é o do preset atual).*/
{{1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {1,1,1,1,1,1,1,1,1,1,1,1}, {0,1,1,1,1,1,1,1,1,1,1,1},
{1,1,0,1,1,1,1,1,1,1,1,1}, {1,0,1,1,1,1,1,1,1,1,1,1}, {0,1,1,1,1,1,1,1,1,1,1,1}, {0,1,1,1,1,1,1,1,1,1,1,1}};

byte funcaoAtual;  //Verifica qual a função do botão pressionado

MIDI_CREATE_DEFAULT_INSTANCE();

void setup() {

  for (byte i = 0; i < qntBotoes; i++) {
    pinMode(pinoBotoes[i], INPUT_PULLUP); //Inicia os botões como entrada de sinal
    pinMode(estadoLed[0][i], OUTPUT); //Inicia os LED's vermelhos como saída
  }

  MIDI.begin(MIDI_CHANNEL_OMNI); // Inicia a comunicação MIDI
  Serial.begin(31250); // Inicia a comunicação Serial

}

void loop() {

  unsigned long botaoPressAtt = millis(); //Timer que verifica se o botão foi pressionado novamente. Caso sim, o timer é zerado.

  /*A condição abaixo, calcula se o tempo entre a ultima vez que o botão foi pressionado e a atual vez que ele foi pressionado,
  é maior que o timer definido em "timerBotao", nesse caso 5. Caso seja maior, ele está apto a mandar a próxima mensagem MIDI.
  Caso negativo, ele vai ignorar a pressionada. Como eu disse acima, esse timer é relativo. 5 é um número mínimo, mas se as mensagens
  continuarem sendo várias em apenas uma pressionada, é necessário aumentar o timer.*/ 
  if (botaoPressAtt - botaoPressUlt >= timerBotao) {
      
      for (byte i =0; i < qntBotoes; i++) {
      
      travaBotao[i] = digitalRead(pinoBotoes[i]);/*Verifica todos os botões e verifica se ele foi pressionado*/
      
      }
        
        for (byte i=0; i < qntBotoes; i++) { /*Aqui, lemos os botões e executamos ações de acordo com o que foi definido em sua função.*/ 
           
           funcaoAtual = funcao[i];
            
            /*Essa condição abaixo verifica se o estado do botão é diferente desde a ultima vez que foi pressionado.
            Caso positivo, ele segue e executa uma ação.*/
            if (travaBotao[i] != ultimaTravaBotao[i]) {

              switch(funcaoAtual) { /*Ao pressionar um botão, ele escolhe a ação que foi definida em "funcao[qntBotoes]" e a executa.*/

                case Snapshot: /*Ação que envia um "Control Change" fixo (64), juntamente com sua "velocity" (intensidade).
                Essa combinação troca os Snapshots.
                O que são Snapshots? São como presets, porém eles apenas Ligam/Desligam efeitos, diferente de presets, que podem mudar
                parametros dos pedais.
                Essa combinação (64 + velocity), é apenas para o VST Helix Native. Em outros VST's, é necessário ver o manual para ver
                se Snapshots são possiveis e como serem implementados.y*/

                {

                  byte esteBotao = pinoBotoes[i]; /*Aqui criamos uma variável que é igual ao botão pressionado. Após pressionado, o código
                  verifica todos os botões que estão definidos como "Snapshot" e caso esse botão não seja o mesmo que o botão pressionado,
                  o LED é desligado*/
                  
                  if (estadoBotao[i] != 2) {
                  MIDI.sendControlChange(comandoMidi[i], snapshot[i], 1);
                  estadoBotao[i] = 0;
                  botaoPressUlt = botaoPressAtt;
                  }

                    for (byte j = 0; j < qntBotoes; j++) {
                      if (funcao[j] == Snapshot) {
                        if (esteBotao == pinoBotoes[j]) {
                          digitalWrite(estadoLed[0][j], 0)
                        }
                        else {
                          digitalWrite(estadoLed[0][j], 1)
                        }
                      }
                    }

                  ultimaTravaBotao[i] = travaBotao[i];
                  break;
                  
                }
                
              }
              
            }

            if(estadoBotao[i] == 2) {  
              for (byte j = 0; j < qntBotoes; j++) {
      
                estadoBotao[j] = definirEstadoBotao[i][j];
                
              }            
            
           }
           
        }
        
    }

}
