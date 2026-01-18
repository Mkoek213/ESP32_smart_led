package com.example.iot.backend.mqtt;

import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.bouncycastle.cert.X509CertificateHolder;
import org.bouncycastle.cert.jcajce.JcaX509CertificateConverter;
import org.bouncycastle.jce.provider.BouncyCastleProvider;
import org.bouncycastle.openssl.PEMKeyPair;
import org.bouncycastle.openssl.PEMParser;
import org.bouncycastle.openssl.jcajce.JcaPEMKeyConverter;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.integration.annotation.ServiceActivator;
import org.springframework.integration.channel.DirectChannel;
import org.springframework.integration.dsl.IntegrationFlow;
import org.springframework.integration.mqtt.core.DefaultMqttPahoClientFactory;
import org.springframework.integration.mqtt.core.MqttPahoClientFactory;
import org.springframework.integration.mqtt.inbound.MqttPahoMessageDrivenChannelAdapter;
import org.springframework.integration.mqtt.outbound.MqttPahoMessageHandler;
import org.springframework.integration.mqtt.support.DefaultPahoMessageConverter;
import org.springframework.messaging.MessageChannel;
import org.springframework.messaging.MessageHandler;

import javax.net.ssl.KeyManagerFactory;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSocketFactory;
import javax.net.ssl.TrustManagerFactory;
import java.io.FileReader;
import java.security.KeyPair;
import java.security.KeyStore;
import java.security.Security;
import java.security.cert.Certificate;
import java.security.cert.X509Certificate;

@Slf4j
@Configuration
@RequiredArgsConstructor
public class MqttConfiguration {

    private static final String CLIENT_ID = "iot-backend";
    private static final String[] INBOUND_TOPICS = {
            "smart-led/device/+/telemetry",
            "smart-led/device/+/status"
    };
    private static final int COMPLETION_TIMEOUT = 5000;
    private static final int QOS = 1;

    private final MqttProperties mqttProperties;

    @Bean
    public MqttConnectOptions mqttConnectOptions() {
        MqttConnectOptions options = new MqttConnectOptions();
        options.setServerURIs(new String[]{mqttProperties.getBrokerUrl()});

        if (mqttProperties.getCredentials() != null) {
            options.setUserName(mqttProperties.getCredentials().getUsername());
            options.setPassword(mqttProperties.getCredentials().getPassword().toCharArray());
        }

        if (mqttProperties.getSsl() != null && mqttProperties.getSsl().getCaCrtFile() != null) {
            try {
                options.setSocketFactory(getSocketFactory(
                        mqttProperties.getSsl().getCaCrtFile(),
                        mqttProperties.getSsl().getCrtFile(),
                        mqttProperties.getSsl().getKeyFile()
                ));
            } catch (Exception e) {
                log.error("Error configuring MQTT SSL/TLS", e);
                throw new RuntimeException("Failed to configure MQTT SSL", e);
            }
        }

        options.setCleanSession(false);
        return options;
    }

    private SSLSocketFactory getSocketFactory(String caCrtFile, String crtFile, String keyFile) throws Exception {
        Security.addProvider(new BouncyCastleProvider());

        // Load CA certificate
        PEMParser parser = new PEMParser(new FileReader(caCrtFile));
        X509CertificateHolder caCertHolder = (X509CertificateHolder) parser.readObject();
        parser.close();
        X509Certificate caCert = new JcaX509CertificateConverter().setProvider("BC").getCertificate(caCertHolder);

        // Load Client certificate
        parser = new PEMParser(new FileReader(crtFile));
        X509CertificateHolder certHolder = (X509CertificateHolder) parser.readObject();
        parser.close();
        X509Certificate cert = new JcaX509CertificateConverter().setProvider("BC").getCertificate(certHolder);

        // Load Client Private Key
        parser = new PEMParser(new FileReader(keyFile));
        Object keyObject = parser.readObject();
        parser.close();

        KeyPair keyPair;
        if (keyObject instanceof PEMKeyPair) {
            keyPair = new JcaPEMKeyConverter().setProvider("BC").getKeyPair((PEMKeyPair) keyObject);
        } else {
            throw new IllegalArgumentException("Unsupported key format: " + keyObject.getClass());
        }

        // CA Certificate TrustStore
        KeyStore caKeyStore = KeyStore.getInstance(KeyStore.getDefaultType());
        caKeyStore.load(null, null);
        caKeyStore.setCertificateEntry("ca-certificate", caCert);
        TrustManagerFactory tmf = TrustManagerFactory.getInstance(TrustManagerFactory.getDefaultAlgorithm());
        tmf.init(caKeyStore);

        // Client Certificate KeyStore
        KeyStore clientKeyStore = KeyStore.getInstance(KeyStore.getDefaultType());
        clientKeyStore.load(null, null);
        clientKeyStore.setCertificateEntry("certificate", cert);
        clientKeyStore.setKeyEntry("private-key", keyPair.getPrivate(), "password".toCharArray(), new Certificate[]{cert});
        KeyManagerFactory kmf = KeyManagerFactory.getInstance(KeyManagerFactory.getDefaultAlgorithm());
        kmf.init(clientKeyStore, "password".toCharArray());

        // SSL Context
        SSLContext context = SSLContext.getInstance("TLSv1.2");
        context.init(kmf.getKeyManagers(), tmf.getTrustManagers(), null);

        return context.getSocketFactory();
    }

    @Bean
    public MqttPahoClientFactory mqttClientFactory(MqttConnectOptions mqttConnectOptions) {
        DefaultMqttPahoClientFactory factory = new DefaultMqttPahoClientFactory();
        factory.setConnectionOptions(mqttConnectOptions);
        return factory;
    }

    @Bean
    public MessageChannel mqttInboundChannel() {
        return new DirectChannel();
    }

    @Bean
    public MessageChannel mqttOutboundChannel() {
        return new DirectChannel();
    }

    @Bean
    public IntegrationFlow mqttInbound(MqttPahoMessageDrivenChannelAdapter mqttInboundAdapter) {
        return IntegrationFlow.from(mqttInboundAdapter)
                .channel(mqttInboundChannel())
                .get();
    }

    @Bean
    public MqttPahoMessageDrivenChannelAdapter mqttInboundAdapter(MqttPahoClientFactory mqttClientFactory) {
        var adapter = new MqttPahoMessageDrivenChannelAdapter(CLIENT_ID + "-in", mqttClientFactory, INBOUND_TOPICS);
        adapter.setCompletionTimeout(COMPLETION_TIMEOUT);
        adapter.setConverter(new DefaultPahoMessageConverter());
        adapter.setQos(QOS);
        return adapter;
    }

    @Bean
    @ServiceActivator(inputChannel = "mqttOutboundChannel")
    public MessageHandler mqttOutboundHandler(MqttPahoClientFactory mqttClientFactory) {
        MqttPahoMessageHandler messageHandler = new MqttPahoMessageHandler(CLIENT_ID + "-out", mqttClientFactory);
        messageHandler.setAsync(true);
        messageHandler.setDefaultQos(QOS);
        return messageHandler;
    }
}
