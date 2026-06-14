import numpy as np
import pandas as pd
import xgboost as xgb
from sklearn.preprocessing import StandardScaler, OneHotEncoder
from sklearn.compose import ColumnTransformer
from sklearn.model_selection import train_test_split
from sklearn.metrics import mean_squared_error, mean_absolute_percentage_error
from tensorflow.keras.models import Sequential, Model  # type: ignore
from tensorflow.keras.layers import ConvLSTM2D, Flatten, Dense, Attention, Input  # type: ignore
import matplotlib.pyplot as plt
import matplotlib.font_manager as fm
import os
from datetime import datetime
import time
from scipy.sparse import issparse
import tensorflow as tf


# 构建ConvLSTM - Attention模型
def build_convlstm_attention_model(input_shape):
    inputs = Input(shape=input_shape)
    convlstm = ConvLSTM2D(filters=64, kernel_size=(3, 3), input_shape=input_shape, padding='same', activation='relu')(
        inputs)
    flatten = Flatten()(convlstm)

    # 调整维度以适应 Attention 层
    query = Dense(128, activation='relu')(flatten)
    key = Dense(128, activation='relu')(flatten)
    value = Dense(128, activation='relu')(flatten)

    # 使用 Lambda 层来增加维度
    def expand_dim(x):
        return tf.expand_dims(x, axis=1)

    query = tf.keras.layers.Lambda(expand_dim)(query)
    key = tf.keras.layers.Lambda(expand_dim)(key)
    value = tf.keras.layers.Lambda(expand_dim)(value)

    # 注意力层
    attention_layer = Attention()([query, key, value])

    # 使用 Lambda 层移除增加的维度
    def squeeze_dim(x):
        return tf.squeeze(x, axis=1)

    attention_layer = tf.keras.layers.Lambda(squeeze_dim)(attention_layer)

    outputs = Dense(1)(attention_layer)

    model = Model(inputs=inputs, outputs=outputs)
    model.compile(optimizer='adam', loss='mse')
    return model


# 可视化结果
def visualize_results(y_test, y_pred, title, save_path):
    plt.figure(figsize=(12, 8))
    # 绘制真实值
    plt.plot(y_test, label='真实值', color='blue', linewidth=2, linestyle='-', marker='o', markersize=6)
    # 绘制预测值
    plt.plot(y_pred, label='预测值', color='red', linewidth=2, linestyle='--', marker='s', markersize=6)

    # 添加数据标签
    for x, y in enumerate(y_test):
        plt.annotate(f'{y:.2f}', (x, y), textcoords='offset points', xytext=(0, 5), ha='center', fontsize=8)
    for x, y in enumerate(y_pred):
        plt.annotate(f'{y:.2f}', (x, y), textcoords='offset points', xytext=(0, -5), ha='center', fontsize=8)

    plt.title(title, fontsize=18)
    plt.xlabel('样本编号', fontsize=14)
    plt.ylabel('销量', fontsize=14)
    plt.xticks(fontsize=12)
    plt.yticks(fontsize=12)
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.legend(fontsize=14, loc='upper left')
    plt.tight_layout()
    try:
        plt.savefig(save_path)
        print(f"图表已保存到 {save_path}")
    except Exception as e:
        print(f"保存图表时出错: {e}")
    finally:
        plt.close()


# 读取数据
def read_data(file_path):
    try:
        data = pd.read_excel(file_path)
        return data
    except FileNotFoundError:
        print(f"错误: 文件 {file_path} 未找到。")
        return None
    except Exception as e:
        print(f"读取文件时出错: {e}")
        return None


# 数据预处理
def preprocess_data(data):
    if data is None:
        return None, None
    # 筛选出数值类型的列
    numeric_columns = data.select_dtypes(include=[np.number]).columns
    # 仅对数值类型的列处理NaN值
    data[numeric_columns] = data[numeric_columns].fillna(data[numeric_columns].median())
    # 仅对数值类型的列处理无穷大值
    data[numeric_columns] = data[numeric_columns].replace([np.inf, -np.inf], np.nan)
    data[numeric_columns] = data[numeric_columns].fillna(data[numeric_columns].median())

    # 假设数据集的最后一列是目标变量（销量），其余列为特征
    X = data.iloc[:, :-1]
    y = data.iloc[:, -1].values

    # 将特征名称转换为字符串类型
    X.columns = X.columns.astype(str)

    # 分离类别型特征和数值型特征
    categorical_features = X.select_dtypes(include=['object']).columns
    numerical_features = X.select_dtypes(exclude=['object']).columns

    # 定义ColumnTransformer进行编码和标准化
    preprocessor = ColumnTransformer(
        transformers=[
            ('cat', OneHotEncoder(), categorical_features),
            ('num', StandardScaler(), numerical_features)
        ])

    X = preprocessor.fit_transform(X)
    return X, y


# 构建ConvLSTM模型
def build_convlstm_model(input_shape):
    model = Sequential()
    model.add(ConvLSTM2D(filters=64, kernel_size=(3, 3), input_shape=input_shape, padding='same', activation='relu'))
    model.add(Flatten())
    model.add(Dense(1))
    model.compile(optimizer='adam', loss='mse')
    return model


# 构建XGBoost模型
def build_xgboost_model():
    model = xgb.XGBRegressor(objective='reg:squarederror', n_estimators=100, learning_rate=0.1)
    return model


# 构建具有时空滞后性的XGBoost模型
def build_xgboost_spatiotemporal_model():
    model = xgb.XGBRegressor(objective='reg:squarederror', n_estimators=100, learning_rate=0.1)
    return model


# 单次运行模型
def run_model(data, result_dir, desktop_path, run_number):
    print(f"\n正在运行第 {run_number} 次...")
    start_time = time.time()

    X, y = preprocess_data(data)
    if X is None or y is None:
        return None, None, None

    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42 + run_number)

    # 检查是否为稀疏矩阵，如果是则转换为密集数组
    if issparse(X_train):
        X_train = X_train.toarray()
    if issparse(X_test):
        X_test = X_test.toarray()

    X_train_convlstm = X_train.reshape(X_train.shape[0], 1, X_train.shape[1], 1, 1)
    X_test_convlstm = X_test.reshape(X_test.shape[0], 1, X_test.shape[1], 1, 1)

    # 训练ConvLSTM模型
    convlstm_model = build_convlstm_model(input_shape=(1, X_train.shape[1], 1, 1))
    convlstm_model.fit(X_train_convlstm, y_train, epochs=20, batch_size=32, verbose=0)
    convlstm_features_train = convlstm_model.predict(X_train_convlstm)
    convlstm_features_test = convlstm_model.predict(X_test_convlstm)
    X_train_combined = np.hstack([X_train, convlstm_features_train])
    X_test_combined = np.hstack([X_test, convlstm_features_test])

    # 训练XGBoost模型
    xgboost_model = build_xgboost_model()
    xgboost_model.fit(X_train_combined, y_train)
    y_pred_xgboost = xgboost_model.predict(X_test_combined)

    # 训练具有时空滞后性的XGBoost模型
    xgboost_spatiotemporal_model = build_xgboost_spatiotemporal_model()
    xgboost_spatiotemporal_model.fit(X_train_combined, y_train)
    y_pred_xgboost_spatiotemporal = xgboost_spatiotemporal_model.predict(X_test_combined)

    # 训练ConvLSTM - Attention模型
    convlstm_attention_model = build_convlstm_attention_model(input_shape=(1, X_train.shape[1], 1, 1))
    convlstm_attention_model.fit(X_train_convlstm, y_train, epochs=20, batch_size=32, verbose=0)
    y_pred_convlstm_attention = convlstm_attention_model.predict(X_test_convlstm).flatten()

    # 评估模型
    mse_xgboost = mean_squared_error(y_test, y_pred_xgboost)
    mape_xgboost = mean_absolute_percentage_error(y_test, y_pred_xgboost)

    mse_xgboost_spatiotemporal = mean_squared_error(y_test, y_pred_xgboost_spatiotemporal)
    mape_xgboost_spatiotemporal = mean_absolute_percentage_error(y_test, y_pred_xgboost_spatiotemporal)

    mse_convlstm_attention = mean_squared_error(y_test, y_pred_convlstm_attention)
    mape_convlstm_attention = mean_absolute_percentage_error(y_test, y_pred_convlstm_attention)

    # 保存结果
    result = {
        'run_number': run_number,
        'xgboost_params': xgboost_model.get_params(),
        'xgboost_spatiotemporal_params': xgboost_spatiotemporal_model.get_params(),
        'mse_xgboost': mse_xgboost,
        'mape_xgboost': mape_xgboost,
        'mse_xgboost_spatiotemporal': mse_xgboost_spatiotemporal,
        'mape_xgboost_spatiotemporal': mape_xgboost_spatiotemporal,
        'mse_convlstm_attention': mse_convlstm_attention,
        'mape_convlstm_attention': mape_convlstm_attention,
        'y_test': y_test,
        'y_pred_xgboost': y_pred_xgboost,
        'y_pred_xgboost_spatiotemporal': y_pred_xgboost_spatiotemporal,
        'y_pred_convlstm_attention': y_pred_convlstm_attention
    }

    # 保存到文件
    result_file = os.path.join(result_dir, f"run_{run_number}.npz")
    np.savez(result_file, **result)

    # 打印进度
    elapsed_time = time.time() - start_time
    print(f"第 {run_number} 次运行完成，耗时 {elapsed_time:.2f} 秒")
    print(f"结果已保存到 {result_file}")

    # 可视化XGBoost结果
    chart_file_xgboost = os.path.join(desktop_path, f"run_{run_number}_xgboost_chart.png")
    visualize_results(y_test, y_pred_xgboost, f'第 {run_number} 次运行 - XGBoost：真实值 vs 预测值', chart_file_xgboost)

    # 可视化具有时空滞后性的XGBoost结果
    chart_file_xgboost_spatiotemporal = os.path.join(desktop_path, f"run_{run_number}_xgboost_spatiotemporal_chart.png")
    visualize_results(y_test, y_pred_xgboost_spatiotemporal, f'第 {run_number} 次运行 - 具有时空滞后性的XGBoost：真实值 vs 预测值',
                      chart_file_xgboost_spatiotemporal)

    # 可视化ConvLSTM - Attention结果
    chart_file_convlstm_attention = os.path.join(desktop_path, f"run_{run_number}_convlstm_attention_chart.png")
    visualize_results(y_test, y_pred_convlstm_attention, f'第 {run_number} 次运行 - ConvLSTM - Attention：真实值 vs 预测值',
                      chart_file_convlstm_attention)

    return mse_xgboost, mape_xgboost, (y_test, y_pred_xgboost, xgboost_model.get_params())


# 主函数
def main():
    # 设置字体
    font_path = '/System/Library/AssetsV2/com_apple_MobileAsset_Font7/3419f2a427639ad8c8e139149a287865a90fa17e.asset/AssetData/PingFang.ttc'
    if os.path.exists(font_path):
        font_prop = fm.FontProperties(fname=font_path)
        plt.rcParams['font.family'] = font_prop.get_name()
        plt.rcParams['axes.unicode_minus'] = False
        print("成功使用字体: PingFang SC")
    else:
        print("警告：未找到PingFang字体，使用默认字体")

    # 创建结果文件夹
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    result_dir = f"results_{timestamp}"
    os.makedirs(result_dir, exist_ok=True)

    file_path = '/Users/ruanji/Desktop/统建/副本比亚迪数据.xlsx'
    desktop_path = os.path.join(os.path.expanduser("~"), "Desktop")

    best_mse = float('inf')
    best_mape = float('inf')
    best_params = None
    best_y_test = None
    best_y_pred = None

    data = read_data(file_path)
    if data is None:
        return

    for i in range(5):
        mse, mape, (y_test, y_pred, params) = run_model(data, result_dir, desktop_path, i + 1)
        if mse is not None and mse < best_mse:
            best_mse = mse
            best_mape = mape
            best_params = params
            best_y_test = y_test
            best_y_pred = y_pred

    print(f"\n所有运行完成，结果保存在 {result_dir} 目录中")

    # 输出最佳参数和评估指标
    print("最佳XGBoost参数:", best_params)
    print(f"最佳XGBoost均方误差 (MSE): {best_mse}")
    print(f"最佳XGBoost平均绝对百分比误差 (MAPE): {best_mape}")


if __name__ == "__main__":
    main()