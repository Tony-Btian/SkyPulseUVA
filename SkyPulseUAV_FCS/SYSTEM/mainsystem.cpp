#include "mainsystem.h"

MainSystem::MainSystem()
{
    GPIO_Initial();
    Function_Initial();
    UI_Initial();
}

MainSystem::~MainSystem()
{
    qWarning("System Quit");
    delete motor_pwm;
    delete readTimer;
    gpioTerminate();
}

void MainSystem::UI_Initial()
{
    // Create Pushbutton
    QPushButton *function_button1 = new QPushButton("Function1", this);
    QPushButton *function_button2 = new QPushButton("Function2", this);
    QPushButton *function_button3 = new QPushButton("Function3", this);

    // Create SpinBoxes
    QSpinBox *spinBoxPitch = new QSpinBox(this);
    spinBoxPitch->setRange(0, 360);  // Setting the range of Pitch
    spinBoxPitch->setSuffix(" D");   // Adding a Degree Suffix
    QSpinBox *spinBoxYaw = new QSpinBox(this);
    spinBoxYaw->setRange(0, 360);
    spinBoxYaw->setSuffix(" D");
    QSpinBox *spinBoxRoll = new QSpinBox(this);
    spinBoxRoll->setRange(0, 360);
    spinBoxRoll->setSuffix(" D");
    QSpinBox *spinBoxThrust = new QSpinBox(this);
    spinBoxThrust->setRange(0, 100);  // Assuming Thrust is a percentage
    spinBoxThrust->setSuffix(" %");

    QPushButton *writeButton = new QPushButton("Write", this);

    // Creating and Configuring DroneConfigManager
    QStringList headers = {"Pitch", "Yaw", "Roll", "Thrust"};
    configManager = new DroneConfigManager("FlightConfig.txt", headers);

    // Master layout
    QVBoxLayout *mainLayout = new QVBoxLayout();
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QHBoxLayout *inputLayout = new QHBoxLayout();

    // Adding buttons to a layout
    buttonLayout->addWidget(function_button1);
    buttonLayout->addWidget(function_button2);
    buttonLayout->addWidget(function_button3);

    // Add SpinBox to Layout
    inputLayout->addWidget(spinBoxPitch);
    inputLayout->addWidget(spinBoxYaw);
    inputLayout->addWidget(spinBoxRoll);
    inputLayout->addWidget(spinBoxThrust);
    inputLayout->addWidget(writeButton);

    mainLayout->addLayout(buttonLayout);
    mainLayout->addLayout(inputLayout);

    setLayout(mainLayout);

    // Connecting signals and slots
    connect(function_button1, &QPushButton::clicked, this, &MainSystem::onButtonClicked_Function1);
    connect(function_button2, &QPushButton::clicked, this, &MainSystem::toggleTimer);
//    connect(function_button2, &QPushButton::clicked, this, &MainSystem::onButtonClicked_Function2);
    connect(function_button3, &QPushButton::clicked, this, &MainSystem::onButtonClicked_Function3);


    connect(spinBoxPitch, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, [this, spinBoxPitch]() {
                int pitch_value = (255 * spinBoxPitch->value()) / 360;
                qDebug() << pitch_value;
                flight_control->setPitch(pitch_value);
            });
    connect(spinBoxYaw, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, [this, spinBoxYaw]() {
                int yaw_value = (255 * spinBoxYaw->value()) / 360;
                qDebug() << yaw_value;
                flight_control->setYaw(yaw_value);
            });
    connect(spinBoxRoll, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, [this, spinBoxRoll]() {
                int roll_value = (255 * spinBoxRoll->value()) / 360;
                qDebug() << roll_value;
                flight_control->setRoll(roll_value);
            });
    connect(spinBoxThrust, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, [this, spinBoxThrust]() {
                int thrust_value = (255 * spinBoxThrust->value()) / 100;
                qDebug() << thrust_value;
                flight_control->setThrust(thrust_value);
            });

    connect(writeButton, &QPushButton::clicked, this, [this, spinBoxPitch, spinBoxYaw, spinBoxRoll, spinBoxThrust](){
        QVector<uint8_t> data = {
            static_cast<uint8_t>(spinBoxPitch->value()),
            static_cast<uint8_t>(spinBoxYaw->value()),
            static_cast<uint8_t>(spinBoxRoll->value()),
            static_cast<uint8_t>(spinBoxThrust->value())
        };
        configManager->writeData(data);
        qDebug() << "Flight configuration saved using DroneConfigManager.";
    });

    // System Information Display
    qWarning("SkyPluse UAV System Initialized");
    qWarning("===============================");
    qDebug() << "Main System Initialized Thread:" << QThread::currentThreadId();
}

void MainSystem::GPIO_Initial()
{
    // Initial GPIO
    while(true){
        if (gpioInitialise() < 0) {
            qWarning("Failed to initialize pigpio. Retrying in 1 second...");
            QThread::sleep(2); // wait for 1 s
            continue; // continue loop
        }
        qWarning("Pigpio initialized successfully");
        notifyObservers(true);
        break; // Successful initialization, jump out of the loop
    }
}

void MainSystem::Function_Initial()
{
    /*Sensors*/
    sensor_manager = new SensorManager();
    tcp_server = new TCP();
    motor_pwm = new MotorPWM(this);
    flight_control = new FlightControl(0, 0, 0, 0);
    readTimer = new QTimer(this);
    video_streamer = new VideoStreamer();

    connect(this, &MainSystem::sig_readAllSensorDataTest, sensor_manager, &SensorManager::ReadAllSensorData);
    connect(tcp_server, &TCP::sig_sendPWMSignal, motor_pwm, &MotorPWM::setMotorPWMSignal);
    connect(tcp_server, &TCP::sig_sendFlightControlSignal, this, &MainSystem::SetFlightControl);
    connect(tcp_server, &TCP::sig_takeOffSignal, this, &MainSystem::startTimer);
    connect(tcp_server, &TCP::sig_landingSignal, this, &MainSystem::stopTimer);
    connect(flight_control, &FlightControl::sig_sendMotorData, motor_pwm, &MotorPWM::setMotorPWMSignal);
    connect(readTimer, &QTimer::timeout, sensor_manager, &SensorManager::ReadAllSensorData);
    connect(sensor_manager, &SensorManager::sig_sendMessage64Bytes, tcp_server, &TCP::sendMessage64Bytes);
    connect(sensor_manager, &SensorManager::updateSensorData, this, [](float ax, float ay, float az, float gx, float gy, float gz, double pressure, double temperature, int x, int y, int z, double heading){
            qDebug() << "Received sensor data in main thread";
            qDebug() << "Acceleration:" << ax << "," << ay << "," << az;
            qDebug() << "Gyroscope:" << gx << "," << gy << "," << gz;
            qDebug() << "Presure:" << pressure;
            qDebug() << "Temperature:" << temperature;
            qDebug() << "Direction:" << x << y << z;
            qDebug() << "Heading Degree:" << heading;
        });
}

void MainSystem::onButtonClicked_Function1()
{
    emit sig_readAllSensorDataTest();
    // Atom action
}

void MainSystem::onButtonClicked_Function2()
{
    qWarning("Function Button 2 Click");
}

void MainSystem::onButtonClicked_Function3()
{

}

void MainSystem::onSpinBoxPitch_valueChanged(int value)
{

}

void MainSystem::onSpinBoxRoll_valueChanged(int value)
{

}

void MainSystem::onSpinBoxThrust_valueChanged(int value)
{

}

void MainSystem::onSpinBoxYaw_valueChanged(int value)
{

}

void MainSystem::toggleTimer()
{
    if (timerRunning) {
        startTimer();
        qDebug() << "Timer stopped";
    } else {
        startTimer();
        qDebug() << "Timer started";
    }
    timerRunning = !timerRunning;
}

void MainSystem::startTimer()
{
    readTimer->start(100);
}

void MainSystem::stopTimer()
{
    readTimer->stop();
}

void MainSystem::SetFlightControl(const QVector<uint8_t> &flight_control_data)
{
    flight_control->setPitch(flight_control_data.at(0));
    flight_control->setYaw(flight_control_data.at(1));
    flight_control->setRoll(flight_control_data.at(2));
    flight_control->setThrust(flight_control_data.at(3));
}

void MainSystem::SetFlightConfigData(const QVector<quint8> &FlightCofig)
{
    flight_control->setThrust(FlightCofig.at(0));
}
