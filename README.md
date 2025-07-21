![image](https://github.com/user-attachments/assets/f2a5c9b8-6208-4723-8f46-1d74be421827)

# 📊 Projeto: Sistema de Monitoramento Ambiental com Raspberry Pi Pico W

## 📑 Sumário
- [🎯 Objetivos](#-objetivos)
- [📋 Descrição do Projeto](#-descrição-do-projeto)
- [⚙️ Funcionalidades Implementadas](#️-funcionalidades-implementadas)
- [🛠️ Requisitos do Projeto](#️-requisitos-do-projeto)
- [📂 Estrutura do Código](#-estrutura-do-código)
- [🖥️ Como Compilar](#️-como-compilar)
- [🤝 Contribuições](#-contribuições)
- [📽️ Demonstração em Vídeo](#️-demonstração-em-vídeo)
- [💡 Considerações Finais](#-considerações-finais)

## 🎯 Objetivos
- Desenvolver um sistema de monitoramento ambiental utilizando o Raspberry Pi Pico W.
- Medir e exibir em tempo real dados de temperatura, umidade e pressão atmosférica.
- Fornecer uma interface web para visualização dos dados e configuração de limites e offsets.
- Implementar alertas visuais e sonoros para condições ambientais críticas.
- Conectar o dispositivo a uma rede Wi-Fi para acesso remoto.

## 📋 Descrição do Projeto
Este projeto utiliza o Raspberry Pi Pico W para monitorar condições ambientais. Ele emprega os seguintes componentes e funcionalidades:

- **Sensores I2C:**
    - **AHT20:** Para leitura de temperatura e umidade.
    - **BMP280:** Para leitura de pressão atmosférica e temperatura, com cálculo de altitude.
- **Conectividade Wi-Fi:** Utiliza o chip CYW43 embutido no Pico W para conexão à rede.
- **Servidor Web Embarcado:** Permite acesso a dados em tempo real e configuração de parâmetros via navegador.
- **LEDs WS2812 (NeoPixel):** Um painel de 25 LEDs configurado via PIO para exibir padrões visuais.
- **LEDs de Status RGB:** LEDs individuais (Azul, Verde, Vermelho) para indicação rápida do estado ambiental.
- **Buzzers Duplos:** Acionados via PWM para alertas sonoros.
- **Botões:** Para interação local e controle do dispositivo.

## ⚙️ Funcionalidades Implementadas
1.  **Monitoramento Ambiental:**
    - Leitura contínua de temperatura e umidade do sensor AHT20.
    - Leitura contínua de pressão atmosférica e temperatura do sensor BMP280.
    - Cálculo da altitude estimada com base na pressão atmosférica.

2.  **Servidor Web Embarcado:**
    - Fornece páginas HTML interativas para visualização de dados em tempo real (Temperatura, Umidade, Pressão).
    - Exibe gráficos simples dos dados ao longo do tempo.
    - Permite configurar limites mínimos e máximos para temperatura, umidade e pressão via interface web.
    - Permite aplicar offsets de calibração aos valores dos sensores.
    - Endpoint JSON (`/dados`) para acesso programático aos dados dos sensores.
    - Sincronização automática da página web com a página atual selecionada no dispositivo.

3.  **Sistema de Alertas Visuais (LEDs):**
    - **LEDs WS2812 (NeoPixel):** Exibem padrões numéricos coloridos (e.g., "1" em vermelho para temperatura alta, "0" em azul para temperatura baixa, "4" em verde para condições normais, etc.) indicando o tipo de alerta.
    - **LEDs RGB Dedicados:** Acendem em cores específicas (Vermelho, Azul, Amarelo, Ciano, Magenta) para sinalizar diferentes condições críticas (e.g., Vermelho para temperatura alta, Azul para temperatura baixa, Amarelo para umidade baixa).

4.  **Sistema de Alertas Sonoros:**
    - Buzzers duplos acionados via PWM para emitir bips contínuos quando qualquer limite configurado é excedido.
    - O buzzer é desativado quando as condições voltam ao normal.

5.  **Controle por Botões:**
    - **Botão A (GPIO 5):** Alterna entre as diferentes páginas de visualização de dados na interface web (Início, Temperatura, Umidade, Pressão, Calibração).
    - **Botão B (GPIO 6):** Coloca o Raspberry Pi Pico W em modo de bootloader USB, facilitando o upload de novo firmware.

6.  **Calibração e Limites:**
    - Offsets configuráveis para temperatura, umidade e pressão, permitindo ajustar as leituras dos sensores.
    - Limites configuráveis (mínimo e máximo) para temperatura, umidade e pressão, que disparam os alertas.

## 🛠️ Requisitos do Projeto
- **Hardware:**
    - Raspberry Pi Pico W
    - Sensor de Temperatura e Umidade AHT20
    - Sensor de Pressão Barométrica BMP280
    - LEDs WS2812 (NeoPixel) - painel de 25 pixels
    - LEDs RGB (ou LEDs discretos Azul, Verde, Vermelho)
    - 2 Buzzers
    - 2 Botões (com resistor de pull-up externo ou interno)
    - Resistores para LEDs (se necessário)
    - Protoboard e fios jumper para conexões.
- **Software/Bibliotecas:**
    - Raspberry Pi Pico SDK
    - Biblioteca LwIP (para pilha de rede TCP/IP)
    - Drivers para AHT20 e BMP280 (provavelmente inclusos ou adaptados)
    - Driver PIO para WS2812
    - Funções de PWM para controle dos buzzers
    - Funções de interrupção GPIO para botões

## 📂 Estrutura do Repositório
├── main.c              # Código principal do projeto, inclui lógica de sensores, web server, LEDs e buzzers.
├── aht20.h             # Arquivo de cabeçalho para o driver do sensor AHT20.
├── aht20.c             # Implementação do driver do sensor AHT20.
├── bmp280.h            # Arquivo de cabeçalho para o driver do sensor BMP280.
├── bmp280.c            # Implementação do driver do sensor BMP280.
├── ws2812.pio.h        # Arquivo de cabeçalho para o programa PIO dos LEDs WS2812.
├── ws2812.pio          # Programa PIO para controle dos LEDs WS2812.
├── ...                 # Demais arquivos de configuração e bibliotecas do Pico SDK

## 🖥️ Como Compilar
1.  **Clone o repositório:**
    ```bash
    git clone https://github.com/JPSRaccolto/estacao_clima.git
    ```
    (Substitua `<URL_DO_SEU_REPOSITORIO>` pela URL real do seu repositório Git, se houver um.)
2.  **Navegue até o diretório do projeto:**
    ```bash
    cd estacao_clime
    ```
3.  **Configure as credenciais Wi-Fi:**
    No arquivo `main.c`, altere `WIFI_SSID` e `WIFI_PASSWORD` para as credenciais da sua rede Wi-Fi:
    ```c
    #define WIFI_SSID "Seu_SSID_aqui"
    #define WIFI_PASSWORD "Sua_Senha_aqui"
    ```
4.  **Compile o projeto:**
    Certifique-se de que seu ambiente de desenvolvimento para o Raspberry Pi Pico W esteja configurado (e.g., VS Code com a extensão Pico, ou CMake/Make no terminal). Utilize o comando de compilação apropriado para o seu setup.
    Ex: Para CMake:
    ```bash
    mkdir build
    cd build
    cmake ..
    make
    ```
5.  **Carregue o código na placa:**
    Conecte seu Raspberry Pi Pico W ao computador enquanto segura o botão `BOOTSEL`. Ele aparecerá como um dispositivo de armazenamento. Arraste o arquivo `.uf2` gerado na pasta `build` para o dispositivo.

## 🖥️ Método Alternativo (VS Code):
1.  Baixe o repositório como um arquivo ZIP e extraia-o para uma pasta de fácil acesso.
2.  Abra o VS Code e instale a extensão "Pico-Go" ou "Raspberry Pi Pico".
3.  Abra a pasta do projeto no VS Code.
4.  Utilize o ícone "Build" (geralmente um martelo ou engrenagem) da extensão para compilar o projeto.
5.  Com a BitDogLab (ou qualquer Raspberry Pi Pico W) em modo bootloader (segurando BOOTSEL ao conectar), utilize o ícone "Upload" (geralmente uma seta para cima) da extensão para enviar o programa para a sua RP2040.
6.  Após a inicialização, o Pico W se conectará ao Wi-Fi. Você pode encontrar o endereço IP no console serial do VS Code. Acesse o sistema pelo navegador usando este IP.

## 🧑‍💻 Autor
**João Pedro Soares Raccolto**

## 📝 Descrição
Este projeto implementa um sistema de monitoramento ambiental robusto utilizando o Raspberry Pi Pico W. Ele integra sensores de temperatura (AHT20, BMP280), umidade (AHT20) e pressão atmosférica (BMP280) para coletar dados em tempo real. Uma das características centrais é a presença de um servidor web embarcado, que permite aos usuários visualizar os dados sensoriais e ajustar parâmetros como limites de alerta e offsets de calibração através de qualquer navegador conectado à mesma rede Wi-Fi. O sistema também inclui alertas visuais via LEDs WS2812 (NeoPixel) e LEDs RGB de status, além de alertas sonoros por meio de buzzers duplos, garantindo que os usuários sejam notificados sobre condições ambientais críticas. A interação local é possível através de botões que controlam a navegação da interface web ou permitem reiniciar o dispositivo no modo de bootloader.

## 🤝 Contribuições
Este projeto foi desenvolvido por **João Pedro Soares Raccolto**.
Contribuições são bem-vindas! Siga os passos abaixo para contribuir:

1.  Fork este repositório.
2.  Crie uma nova branch:
    ```bash
    git checkout -b minha-feature
    ```
3.  Faça suas modificações e commit:
    ```bash
    git commit -m 'Minha nova feature'
    ```
4.  Envie suas alterações:
    ```bash
    git push origin minha-feature
    ```
5.  Abra um Pull Request.

## 📽️ Demonstração em Vídeo
- O vídeo de demonstração do projeto pode ser visualizado aqui: [Link para o vídeo](https://drive.google.com/file/d/1CtTBlMcizYix0AwHNDmm700G4hOCUKI9/view?usp=sharing)

## 💡 Considerações Finais
Este projeto demonstra a capacidade do Raspberry Pi Pico W em construir sistemas de monitoramento ambiental conectados e interativos. A combinação de sensores, interface web, e sistemas de alerta visual e sonoro oferece uma solução completa para supervisão de ambientes. O servidor web e a funcionalidade de calibração remota aumentam a flexibilidade e usabilidade do sistema. Futuras expansões poderiam incluir o registro de dados em um cartão SD, integração com plataformas de IoT para monitoramento em nuvem, ou a adição de mais tipos de sensores para uma análise ambiental mais abrangente.
