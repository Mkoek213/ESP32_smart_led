package com.karolbystrek.mqttdatahub.configuration;

import lombok.RequiredArgsConstructor;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.integration.channel.DirectChannel;
import org.springframework.integration.dsl.IntegrationFlow;
import org.springframework.integration.mqtt.core.DefaultMqttPahoClientFactory;
import org.springframework.integration.mqtt.core.MqttPahoClientFactory;
import org.springframework.integration.mqtt.inbound.MqttPahoMessageDrivenChannelAdapter;
import org.springframework.integration.mqtt.support.DefaultPahoMessageConverter;
import org.springframework.messaging.MessageChannel;

@Configuration
@RequiredArgsConstructor
public class MqttConfiguration {

    private static final String CLIENT_ID = "iot-backend";
    private static final String TELEMETRY_TOPIC = "customer/+/location/+/device/+/sensor-data";
    private static final int COMPLETION_TIMEOUT = 5000;
    private static final int QOS = 1;

    private final MqttProperties mqttProperties;

    @Bean
    public MqttConnectOptions mqttConnectOptions() {
        MqttConnectOptions options = new MqttConnectOptions();
        options.setServerURIs(new String[]{mqttProperties.getBrokerUrl()});
        options.setUserName(mqttProperties.getCredentials().getUsername());
        options.setPassword(mqttProperties.getCredentials().getPassword().toCharArray());
        return options;
    }

    @Bean
    public MqttPahoClientFactory mqttClientFactory(MqttConnectOptions mqttConnectOptions) {
        DefaultMqttPahoClientFactory factory = new DefaultMqttPahoClientFactory();
        factory.setConnectionOptions(mqttConnectOptions);
        return factory;
    }

    @Bean
    public MessageChannel sensorDataChannel() {
        return new DirectChannel();
    }

    @Bean
    public IntegrationFlow mqttInbound(MqttPahoMessageDrivenChannelAdapter mqttInboundAdapter) {
        return IntegrationFlow.from(mqttInboundAdapter)
                .channel(sensorDataChannel())
                .get();
    }

    @Bean
    public MqttPahoMessageDrivenChannelAdapter mqttInboundAdapter(MqttPahoClientFactory mqttClientFactory) {
        var adapter = new MqttPahoMessageDrivenChannelAdapter(CLIENT_ID, mqttClientFactory, TELEMETRY_TOPIC);
        adapter.setCompletionTimeout(COMPLETION_TIMEOUT);
        adapter.setConverter(new DefaultPahoMessageConverter());
        adapter.setQos(QOS);
        return adapter;
    }
}
