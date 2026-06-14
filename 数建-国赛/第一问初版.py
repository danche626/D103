import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import statsmodels.api as sm
from mpl_toolkits.mplot3d import Axes3D
import matplotlib.font_manager as fm
import os
from scipy.stats import gaussian_kde

plt.rcParams["font.family"] = ["Arial Unicode MS"]  
plt.rcParams['axes.unicode_minus'] = False

plt.style.use('seaborn-v0_8-talk')  

def week_to_float(w):
    """将孕周字符串（如"12w3"）转换为浮点型周数"""
    if pd.isna(w):
        return np.nan
    if isinstance(w, str):
        w = w.strip()
        if not w:
            return np.nan
        if "w" in w:
            parts = w.split("w")
            if len(parts) != 2:
                return np.nan
            w0_str, d_str = parts
            try:
                w0 = int(w0_str) if w0_str else 0  
                d = int(d_str.replace("+", "")) if d_str else 0  
                return w0 + d / 7.0
            except ValueError:
                return np.nan
    try:
        return float(w)
    except (ValueError, TypeError):
        return np.nan

def with_bmi(df):
    """添加BMI计算列并转换孕周格式"""
    df = df.copy()
    df["BMI"] = np.where(
        df["身高"] > 0,
        df["体重"] / ((df["身高"] / 100.0) ** 2),
        np.nan
    )
    df["孕周数"] = df["检测孕周"].apply(week_to_float)
    return df

file_path = r"/Users/ruanji/Desktop/数建/附件.xlsx"
try:
    male = pd.read_excel(file_path, sheet_name="男胎检测数据")
    female = pd.read_excel(file_path, sheet_name="女胎检测数据")
except FileNotFoundError:
    raise FileNotFoundError(f"错误：未找到文件 {file_path}，请检查路径是否正确")
except ValueError as e:
    raise ValueError(f"错误：读取表格失败 - {str(e)}，请检查sheet名称是否正确")

male = with_bmi(male)
female = with_bmi(female)

m = male[["孕周数", "BMI", "Y染色体浓度"]].dropna().copy()
if m.empty:
    raise ValueError("错误：男胎数据预处理后为空，请检查数据格式")

X_lin = sm.add_constant(m[["孕周数", "BMI"]])
y = m["Y染色体浓度"]
lin_model = sm.OLS(y, X_lin).fit()

m["孕周数2"] = m["孕周数"] ** 2
m["BMI2"] = m["BMI"] ** 2
m["交互"] = m["孕周数"] * m["BMI"]
X_quad = sm.add_constant(m[["孕周数", "BMI", "孕周数2", "BMI2", "交互"]])
quad_model = sm.OLS(y, X_quad).fit()

metrics = pd.DataFrame({
    "模型": ["线性", "二次+交互"],
    "R2": [lin_model.rsquared, quad_model.rsquared],
    "Adj-R2": [lin_model.rsquared_adj, quad_model.rsquared_adj],
    "AIC": [lin_model.aic, quad_model.aic],
    "BIC": [lin_model.bic, quad_model.bic],
    "F检验p值": [lin_model.f_pvalue, quad_model.f_pvalue]
}).round(6)

print("模型系数_线性:\n", lin_model.summary2().tables[1].reset_index().rename(columns={"index": "变量"}))
print("\n模型系数_二次交互:\n", quad_model.summary2().tables[1].reset_index().rename(columns={"index": "变量"}))
print("\n模型总体指标对比:\n", metrics)

best = quad_model if quad_model.rsquared_adj >= lin_model.rsquared_adj else lin_model
best_name = "二次+交互" if best is quad_model else "线性"
print(f"\n选择最优模型: {best_name}（更高的调整后R²）")

def init_plot(title, xlabel, ylabel, figsize=(9, 7)):
    """初始化图表基础样式"""
    plt.figure(figsize=figsize)
    plt.title(title, pad=15, fontsize=14)
    plt.xlabel(xlabel, labelpad=10, fontsize=12)
    plt.ylabel(ylabel, labelpad=10, fontsize=12)
    return plt.gcf()

fig = init_plot(
    title="图1 孕周数与Y染色体浓度关系",
    xlabel="孕周数（周）",
    ylabel="Y染色体浓度"
)
plt.scatter(m["孕周数"], m["Y染色体浓度"], s=20, alpha=1.0,
           c="#52738D", edgecolors="#19D2AA", linewidth=0.5)
p = np.polyfit(m["孕周数"], m["Y染色体浓度"], 2)
xx = np.linspace(m["孕周数"].min(), m["孕周数"].max(), 200)
yy = p[0] * xx ** 2 + p[1] * xx + p[2]
plt.plot(xx, yy, linewidth=3, c="#604317", label="拟合曲线")
plt.axhline(0.04, linestyle="--", linewidth=2, c="#A22E4F", label="4%阈值")
plt.legend()
plt.grid(alpha=0.3)
plt.tight_layout()
plt.show()

fig = init_plot(
    title="图2 BMI与Y染色体浓度分布（颜色和形状区分密度）",
    xlabel="BMI",
    ylabel="Y染色体浓度"
)

xy = np.vstack([m["BMI"], m["Y染色体浓度"]])
z = gaussian_kde(xy)(xy)
density_bins = pd.qcut(z, q=3, labels=[0, 1, 2])  

colors = ['#1E88E5', '#FFC107', '#E53935']  
markers = ['o', 's', '^']  
sizes = [20, 50, 80]       

for i in [0, 1, 2]:
    mask = density_bins == i
    plt.scatter(
        m["BMI"][mask], m["Y染色体浓度"][mask],
        marker=markers[i],
        s=sizes[i],
        c=colors[i],
        edgecolor='black',
        alpha=0.7,
        linewidth=1.2,
        label=f'密度等级 {i+1}'
    )

plt.axhline(0.04, linestyle="--", linewidth=2, c='#F44336')
plt.legend(title="密度区分")
plt.grid(alpha=0.3)
plt.tight_layout()
plt.show()

fig = plt.figure(figsize=(10, 8))
ax = fig.add_subplot(111, projection='3d')
sc = ax.scatter(m["孕周数"], m["BMI"], m["Y染色体浓度"],
               s=50, alpha=0.6, c=m["Y染色体浓度"], cmap='plasma',
               edgecolors='white', linewidth=0.3)
cbar = fig.colorbar(sc, ax=ax, pad=0.1)
cbar.set_label("Y染色体浓度", rotation=270, labelpad=20)
ax.set_title("图3 孕周数-BMI-Y染色体浓度三维分布", pad=20)
ax.set_xlabel("孕周数（周）", labelpad=10)
ax.set_ylabel("BMI", labelpad=10)
ax.set_zlabel("Y染色体浓度", labelpad=10)
ax.view_init(elev=30, azim=45)  
plt.tight_layout()
plt.show()

fig = init_plot(
    title=f"图4 模型（{best_name}）预测等高线：孕周×BMI→Y染色体浓度",
    xlabel="孕周数（周）",
    ylabel="BMI"
)
w_grid = np.linspace(m["孕周数"].min(), m["孕周数"].max(), 80)
b_grid = np.linspace(m["BMI"].quantile(0.01), m["BMI"].quantile(0.99), 80)
WW, BB = np.meshgrid(w_grid, b_grid)

if best is quad_model:
    Z = (best.params["const"]
         + best.params["孕周数"] * WW
         + best.params["BMI"] * BB
         + best.params["孕周数2"] * (WW ** 2)
         + best.params["BMI2"] * (BB ** 2)
         + best.params["交互"] * (WW * BB))
else:
    Z = (best.params["const"]
         + best.params["孕周数"] * WW
         + best.params["BMI"] * BB)

cs = plt.contourf(WW, BB, Z, levels=20, cmap='plasma', alpha=0.8)
plt.contour(WW, BB, Z, levels=10, colors='white', linewidths=0.8) 
threshold = plt.contour(WW, BB, Z, levels=[0.04], colors='#F44336', linewidths=2)
plt.clabel(threshold, inline=True, fmt='4%阈值', fontsize=12)

cbar = plt.colorbar(cs)
cbar.set_label("预测Y染色体浓度", rotation=270, labelpad=15)
plt.grid(alpha=0.2)
plt.tight_layout()
plt.show()

if "Y染色体浓度" in female.columns:
    f = female[["孕周数", "BMI", "Y染色体浓度"]].dropna()
    if len(f) > 0:
        check_stats = f["Y染色体浓度"].describe(percentiles=[0.5, 0.9, 0.99]).to_frame(name="女胎Y浓度分布")
        print("\n女胎Y染色体浓度统计:\n", check_stats)
    else:
        print("\n女胎数据中无有效Y染色体浓度记录")
else:
    print("\n女胎表无Y染色体浓度列，符合预期（女胎无Y染色体）")

def earliest_week_for_threshold(bmi, y_thr=0.04):
    """计算特定BMI下Y浓度首次达到阈值的孕周"""
    ws = np.linspace(m["孕周数"].min(), m["孕周数"].max(), 500)
    if best is quad_model:
        yhat = (best.params["const"]
                + best.params["孕周数"] * ws
                + best.params["BMI"] * bmi
                + best.params["孕周数2"] * (ws ** 2)
                + best.params["BMI2"] * (bmi ** 2)
                + best.params["交互"] * (ws * bmi))
    else:
        yhat = (best.params["const"]
                + best.params["孕周数"] * ws
                + best.params["BMI"] * bmi)
    ws_ok = ws[yhat >= y_thr]
    return ws_ok.min() if ws_ok.size > 0 else np.nan

bmi_quants = m["BMI"].quantile([0.1, 0.25, 0.5, 0.75, 0.9]).round(2)
rec_rows = []
for label, bmi_v in bmi_quants.items():
    w_star = earliest_week_for_threshold(bmi_v)
    rec_rows.append({
        "BMI分位": f"{label * 100}%",
        "BMI值": bmi_v,
        "预测首达4%孕周": round(float(w_star), 2) if not pd.isna(w_star) else "无有效值"
    })
rec_table = pd.DataFrame(rec_rows)
print("\n按BMI分位的首达4%孕周（模型推断）:\n", rec_table)
