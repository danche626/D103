import pandas as pd
import numpy as np
from sklearn.preprocessing import StandardScaler
from sklearn.model_selection import train_test_split
from tensorflow.keras.models import Model
from tensorflow.keras.layers import Input, Conv1D, MaxPooling1D, LSTM, Dense, Dropout, BatchNormalization, Add
from tensorflow.keras.layers import LeakyReLU
from tensorflow.keras.optimizers import RMSprop
from tensorflow.keras.callbacks import EarlyStopping, ReduceLROnPlateau
import matplotlib.pyplot as plt
import matplotlib.font_manager as fm

# 查找系统中可用的中文字体
font_list = fm.findSystemFonts()
chinese_font_names = ['SimHei', 'WenQuanYi Zen Hei', 'PingFang', 'Microsoft YaHei']
chinese_fonts = [font for font in font_list if any(name in font for name in chinese_font_names)]

if chinese_fonts:
    for font_path in chinese_fonts:
        try:
            plt.rcParams['font.family'] = fm.FontProperties(fname=font_path).get_name()
            plt.rcParams['axes.unicode_minus'] = False
            print(f"成功使用字体: {plt.rcParams['font.family']}")
            break
        except Exception as e:
            print(f"尝试使用字体 {font_path} 时出错: {e}")
else:
    print("未找到可用的中文字体，可能会出现中文显示问题。")

# 读取CSV文件
def read_data(file_path):
    data = pd.read_csv(file_path)
    return data

# 数据预处理
def preprocess_data(data):
    data['date'] = pd.to_datetime(data['date'], format='%Y_%m')
    data = data.sort_values(by='date')
    # 调整平滑窗口
    data['sales'] = data['sales'].rolling(window=2).mean().fillna(data['sales'])
    # 提取季节和月份特征
    data['season'] = data['date'].dt.month % 12 // 3 + 1
    data['month'] = data['date'].dt.month
    sales = data[['sales', 'season', 'month']].values
    scaler = StandardScaler()
    scaled_sales = scaler.fit_transform(sales)
    return scaled_sales, scaler

# 创建时间序列数据集
def create_dataset(dataset, look_back=12):
    dataX, dataY = [], []
    for i in range(len(dataset) - look_back):
        a = dataset[i:(i + look_back), :]
        dataX.append(a)
        dataY.append(dataset[i + look_back, 0])
    return np.array(dataX), np.array(dataY)

# 构建CNN - LSTM模型
def build_model(input_shape):
    inputs = Input(shape=input_shape)
    # CNN层
    x = Conv1D(filters=512, kernel_size=3, padding='same')(inputs)
    x = LeakyReLU()(x)
    x = BatchNormalization()(x)
    x = MaxPooling1D(pool_size=2)(x)
    residual = x
    x = Conv1D(filters=1024, kernel_size=3, padding='same')(x)
    x = LeakyReLU()(x)
    x = BatchNormalization()(x)
    x = MaxPooling1D(pool_size=2)(x)
    # 调整残差分支的形状
    residual = Conv1D(filters=1024, kernel_size=1)(residual)
    residual = MaxPooling1D(pool_size=2)(residual)
    x = Add()([x, residual])
    x = Conv1D(filters=2048, kernel_size=3, padding='same')(x)
    x = LeakyReLU()(x)
    x = BatchNormalization()(x)
    x = MaxPooling1D(pool_size=2)(x)
    # LSTM层
    x = LSTM(2048, return_sequences=True)(x)
    x = BatchNormalization()(x)
    x = Dropout(0.2)(x)
    x = LSTM(1024, return_sequences=True)(x)
    x = BatchNormalization()(x)
    x = Dropout(0.2)(x)
    x = LSTM(512)(x)
    x = BatchNormalization()(x)
    x = Dropout(0.2)(x)
    outputs = Dense(1)(x)
    model = Model(inputs=inputs, outputs=outputs)
    model.compile(optimizer=RMSprop(learning_rate=0.005), loss='mse')
    return model

# 主函数
def main():
    file_path = '/Users/ruanji/Desktop/统建/数据/中南地区.csv'
    data = read_data(file_path)
    scaled_sales, scaler = preprocess_data(data)

    look_back = 12
    X, y = create_dataset(scaled_sales, look_back)

    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

    input_shape = (look_back, X.shape[2])
    model = build_model(input_shape)

    early_stopping = EarlyStopping(patience=50, restore_best_weights=True)
    reduce_lr = ReduceLROnPlateau(factor=0.1, patience=20, min_lr=0.000001)

    model.fit(X_train, y_train, epochs=2000, batch_size=32,
              validation_data=(X_test, y_test),
              callbacks=[early_stopping, reduce_lr])

    y_pred = model.predict(X_test)
    # 构造完整的预测数组
    y_pred_full = np.hstack((y_pred, np.zeros((y_pred.shape[0], 2))))
    y_test_full = np.hstack((y_test.reshape(-1, 1), np.zeros((y_test.shape[0], 2))))

    # 进行逆变换
    y_pred = scaler.inverse_transform(y_pred_full)[:, 0].reshape(-1, 1)
    y_test = scaler.inverse_transform(y_test_full)[:, 0].reshape(-1, 1)

    plt.figure(figsize=(12, 6))
    line1 = plt.plot(range(len(y_test)), y_test, label='真实值', color='blue', marker='o')
    line2 = plt.plot(range(len(y_pred)), y_pred, label='预测值', color='orange', linestyle='--', marker='s')

    for i, v in enumerate(y_test):
        plt.text(i, v, str(round(v[0], 2)), color='blue', fontsize=8)

    for i, v in enumerate(y_pred):
        plt.text(i, v, str(round(v[0], 2)), color='orange', fontsize=8)

    plt.title('CNN-LSTM模型预测结果')
    plt.xlabel('样本编号')
    plt.ylabel('销量')
    plt.legend()
    plt.grid(True)
    plt.show()

if __name__ == "__main__":
    main()
    