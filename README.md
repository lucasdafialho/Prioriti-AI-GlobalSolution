# Projeto IoT - Prioriti AI (Global Solution 2025)

## Introdução

Este documento descreve a arquitetura e implementação do sistema de Internet das Coisas (IoT) desenvolvido como parte do projeto **Prioriti AI** para o desafio Global Solution 2025 da FIAP. O objetivo deste sistema IoT é coletar dados relevantes de cenários de risco pós-desastre, simulando informações ambientais, estruturais, de localização e status reportados por pessoas no local. Esses dados são a base para alimentar o modelo de Inteligência Artificial (não abordado em detalhes aqui) que classifica a urgência de cada cenário, auxiliando na priorização de equipes de resgate.

Toda a implementação foi realizada utilizando o simulador online **Wokwi** para os dispositivos e sensores, e o protocolo **MQTT** para comunicação.

## Arquitetura Geral

O fluxo de dados do sistema IoT segue a seguinte arquitetura:

1.  **Dispositivos IoT Simulados (Wokwi):** Três tipos de dispositivos ESP32 simulados coletam dados de sensores virtuais ou respondem a interações (botões).
2.  **Comunicação MQTT:** Os dispositivos publicam seus dados em formato JSON para um broker MQTT público (`broker.hivemq.com`) em tópicos específicos.
3.  **Processamento e Visualização:** Uma aplicação (originalmente Node-RED, depois substituída por dashboards frontend customizados) se inscreve nos tópicos MQTT para receber os dados, processá-los e exibi-los em uma interface de usuário. Esta camada também implementa a **lógica de urgência simulada** para fins de demonstração.

```mermaid
graph LR
    A[Dispositivo 1: Ambiental<br>(Wokwi)] -- MQTT --> C{Broker MQTT<br>(broker.hivemq.com)};
    B[Dispositivo 2: Localização/Pânico<br>(Wokwi)] -- MQTT --> C;
    D[Dispositivo 3: Status<br>(Wokwi)] -- MQTT --> C;
    C -- MQTT --> E[Dashboard<br>(Node-RED ou Frontend Custom)];
    E --> F[Visualização +<br>Urgência Simulada];
```

## Dispositivos IoT Simulados (Wokwi)

Foram desenvolvidos três dispositivos simulados na plataforma Wokwi, utilizando o ESP32 como microcontrolador base.

### 1. Dispositivo 1: Nó Ambiental/Estrutural (EnvNode01)

*   **Descrição:** Monitora condições ambientais e indicadores de instabilidade estrutural em um local fixo.
*   **Hardware Simulado:** ESP32, Sensor DHT22 (Temperatura/Umidade), Sensor Ultrassônico HC-SR04 (Distância/Nível d'água), Acelerômetro/Giroscópio MPU6050 (Vibração).
*   **Código:** `device1_env_structural_mqtt.ino`
*   **Comunicação:**
    *   **Tópico MQTT:** `fiap/gs/envnode01/data`
    *   **Payload Exemplo:**
        ```json
        {
          "deviceId": "EnvNode01_ESP32Client",
          "timestamp": 1678886400,
          "data": {
            "temperature_c": 25.5,
            "humidity_percent": 60.1,
            "distance_cm": 150.3,
            "accelerometer": {"x": 0.1, "y": -0.05, "z": 9.8},
            "gyroscope": {"x": 1.2, "y": -0.5, "z": 0.1}
          }
        }
        ```
    *   **Frequência:** Envio periódico (configurado para 30 segundos no código para demonstração).

### 2. Dispositivo 2: Rastreador de Localização/Pânico (LocationTracker01)

*   **Descrição:** Rastreia a localização de um ponto (fixo ou móvel) e permite o envio de um alerta de pânico imediato.
*   **Hardware Simulado:** ESP32, Módulo GPS NEO-6M, Botão Tátil (Pânico).
*   **Código:** `device2_location_panic.ino`
*   **Comunicação:**
    *   **Tópico MQTT:** `fiap/gs/locationtracker01/data`
    *   **Payload Exemplo (Normal):**
        ```json
        {
          "deviceId": "LocationTracker01_ESP32Client",
          "timestamp": 1678886405,
          "latitude": -23.5505,
          "longitude": -46.6333,
          "alertType": "LocationUpdate"
        }
        ```
    *   **Payload Exemplo (Pânico):**
        ```json
        {
          "deviceId": "LocationTracker01_ESP32Client",
          "timestamp": 1678886410,
          "latitude": -23.5510,
          "longitude": -46.6340,
          "alertType": "PanicButton"
        }
        ```
    *   **Frequência:** Envio de localização periódico (configurado para 60 segundos) e envio imediato ao pressionar o botão de pânico.

### 3. Dispositivo 3: Reportador de Status (StatusReporter01)

*   **Descrição:** Permite que um usuário no local reporte rapidamente um status pré-definido através de botões.
*   **Hardware Simulado:** ESP32, Três Botões Táteis (OK, Ajuda, Perigo).
*   **Código:** `device3_status_reporter.ino`
*   **Comunicação:**
    *   **Tópico MQTT:** `fiap/gs/statusreporter01/data`
    *   **Payload Exemplo (Ajuda):**
        ```json
        {
          "deviceId": "StatusReporter01_ESP32Client",
          "timestamp": 1678886420,
          "statusCode": 2,
          "statusReport": "Assistance Needed"
        }
        ```
    *   **Frequência:** Envio baseado em evento (somente quando um botão é pressionado).

## Comunicação

*   **Protocolo:** MQTT v3.1.1
*   **Broker:** `broker.hivemq.com` (Porta 1883 para MQTT padrão, Porta 8884 para MQTT sobre WebSockets seguros - WSS)
*   **Tópicos:**
    *   `fiap/gs/envnode01/data`
    *   `fiap/gs/locationtracker01/data`
    *   `fiap/gs/statusreporter01/data`

## Simulação (Wokwi)

*   Os três dispositivos são simulados em projetos separados no Wokwi.
*   Cada projeto contém o código `.ino` correspondente e o arquivo `diagram.json` que define os componentes e conexões virtuais.
*   Para executar a simulação, basta abrir cada projeto no Wokwi e iniciar a simulação. Os dispositivos tentarão conectar ao Wi-Fi simulado do Wokwi (`Wokwi-GUEST`) e começarão a publicar dados no broker HiveMQ.

## Processamento e Visualização

Inicialmente, o fluxo de dados foi processado e visualizado usando Node-RED (fluxo em `node_red_flows_with_sim_ia_v2_corrected.json`). Posteriormente, foram desenvolvidos dashboards frontend customizados (`dashboard/`, `dashboard_v2/`, `dashboard_v3/`) que:

*   **Opção A (Conexão Real):** Conectam-se diretamente ao broker HiveMQ via MQTT sobre WebSockets (usando MQTT.js) para receber e exibir os dados em tempo real.
*   **Opção B (Simulação Frontend):** Geram dados falsos internamente via JavaScript (`setInterval`, `Math.random`) para simular o recebimento de dados e atualizar a interface, sem depender de conexão externa (útil para demonstrações offline ou vídeos).

Ambas as opções de dashboard implementam a lógica para calcular e exibir a **Urgência Simulada** com base nos dados (reais ou simulados), fornecendo uma representação visual da classificação que a IA faria.

## Integração com IA (Conceitual)

Embora a implementação atual utilize uma *simulação* da classificação de urgência no Node-RED ou no frontend, a arquitetura foi projetada para que os dados coletados pelos dispositivos IoT e centralizados via MQTT pudessem ser consumidos por um serviço de backend executando o modelo de IA treinado (`rescue_classifier_model.joblib`). Este serviço então publicaria a classificação de urgência real (Baixo, Médio, Alto, Crítico) em um novo tópico MQTT, que seria lido pelo dashboard para exibição final.

