# Gate_mqtt_sonoff_4ch_pro

Sistema de comando para um portão de 2 folhas usando um Sonoff 4ch pro.



######################## Memórias de estado e operação em curso########################
int estado_portao = 2; // 0:Fechado, 1:Aberto, 2:Posicao intermedia
int operacao_em_curso = 0; // 0: Nenhuma, 1:Abrindo, 2:Fechando, 3: Abrindo_gente
######################################Instruções#######################################
abrir_portao();
abrir_gente();
fechar_portao();
################################Topicos de comando MQTT################################
Abrir
Abrir uma folha
Fechar
reset
#################################Topicos de estado MQTT################################
portao_exterior/stat
portao_exterior/oper
