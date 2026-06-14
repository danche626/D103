import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import statsmodels.api as sm
from mpl_toolkits.mplot3d import Axes3D
import matplotlib.font_manager as fm
import os

# 中文显示优化（多字体备选，提高兼容性）
plt.rcParams["font.family"] = ["PingFang SC", "WenQuanYi Micro Hei", "Heiti TC", "SimHei"]
plt.rcParams["axes.unicode_minus"] = False  # 解决负号显示问题

# 读取两张表
file_path = r"/Users/ruanji/Desktop/数建/附件.xlsx"
male = pd.read_excel(file_path, sheet_name="男胎检测数据")
female = pd.read_excel(file_path, sheet_name="女胎检测数据")

# 统一预处理函数
def week_to_float(w):
    if isinstance(w, str) and "w" in w:
        try:
            w0, d = w.split("w")
            d = d.replace("+","")
            d = int(d) if d else 0
            return int(w0) + d/7.0
        except:
            return np.nan
    try:
        return float(w)
    except:
        return np.nan

def with_bmi(df):
    df = df.copy()
    df["BMI"] = df["体重"] / ((df["身高"]/100.0)**2)
    df["孕周数"] = df["检测孕周"].apply(week_to_float)
    return df

male = with_bmi(male)
female = with_bmi(female)

# ———— 仅对男胎做 Y 染色体浓度分析 ————
m = male[["孕周数","BMI","Y染色体浓度"]].dropna().copy()

# 线性模型
X_lin = sm.add_constant(m[["孕周数","BMI"]])
y = m["Y染色体浓度"]
lin_model = sm.OLS(y, X_lin).fit()

# 二次 & 交互模型
m["孕周数2"] = m["孕周数"]**2
m["BMI2"] = m["BMI"]**2
m["交互"] = m["孕周数"]*m["BMI"]
X_quad = sm.add_constant(m[["孕周数","BMI","孕周数2","BMI2","交互"]])
quad_model = sm.OLS(y, X_quad).fit()

# 将关键指标整理为表格
coef_lin = lin_model.summary2().tables[1].reset_index().rename(columns={"index":"变量"})
coef_quad = quad_model.summary2().tables[1].reset_index().rename(columns={"index":"变量"})

metrics = pd.DataFrame({
    "模型":["线性","二次+交互"],
    "R2":[lin_model.rsquared, quad_model.rsquared],
    "Adj-R2":[lin_model.rsquared_adj, quad_model.rsquared_adj],
    "AIC":[lin_model.aic, quad_model.aic],
    "BIC":[lin_model.bic, quad_model.bic],
    "F检验p值":[lin_model.f_pvalue, quad_model.f_pvalue]
})

print("模型系数_线性:\n", coef_lin)
print("\n模型系数_二次交互:\n", coef_quad)
print("\n模型总体指标对比:\n", metrics.round(6))

# 选择较优模型用于可视化（以更高的Adj-R2为准）
best = quad_model if quad_model.rsquared_adj >= lin_model.rsquared_adj else lin_model
best_name = "二次+交互" if best is quad_model else "线性"
print(f"\n选择最优模型: {best_name}（更高的调整后R²）")

# —— 图1：孕周 vs Y（散点 + 多项式拟合曲线）——
plt.figure(figsize=(8,6))
plt.scatter(m["孕周数"], m["Y染色体浓度"], s=10, alpha=0.35)
# 二次多项式拟合（仅基于孕周）
p = np.polyfit(m["孕周数"], m["Y染色体浓度"], 2)
xx = np.linspace(m["孕周数"].min(), m["孕周数"].max(), 200)
yy = p[0]*xx**2 + p[1]*xx + p[2]
plt.plot(xx, yy, linewidth=2)
plt.axhline(0.04, linestyle="--", linewidth=1)  # 4%阈值
plt.title("图1 孕周数与Y染色体浓度关系（带二次拟合）")
plt.xlabel("孕周数（周）")
plt.ylabel("Y染色体浓度")
plt.tight_layout()
plt.show()

# —— 图2：BMI vs Y（六边形密度图，展示高BMI对Y浓度的压制）——
plt.figure(figsize=(8,6))
hb = plt.hexbin(m["BMI"], m["Y染色体浓度"], gridsize=35, mincnt=5)
cb = plt.colorbar(hb)
cb.set_label("计数")
plt.axhline(0.04, linestyle="--", linewidth=1)
plt.title("图2 BMI与Y染色体浓度六边形密度图")
plt.xlabel("BMI")
plt.ylabel("Y染色体浓度")
plt.tight_layout()
plt.show()

# —— 图3：三维散点（孕周、BMI 与 Y）——
fig = plt.figure(figsize=(9,7))
ax = fig.add_subplot(111, projection='3d')
ax.scatter(m["孕周数"], m["BMI"], m["Y染色体浓度"], s=8, alpha=0.6)
ax.set_title("图3 孕周数-BMI-Y染色体浓度三维分布")
ax.set_xlabel("孕周数（周）")
ax.set_ylabel("BMI")
ax.set_zlabel("Y染色体浓度")
plt.tight_layout()
plt.show()

# —— 图4：基于最优模型的预测等高线（周×BMI 网格）——
# 构造网格
w_grid = np.linspace(m["孕周数"].min(), m["孕周数"].max(), 80)
b_grid = np.linspace(m["BMI"].quantile(0.01), m["BMI"].quantile(0.99), 80)
WW, BB = np.meshgrid(w_grid, b_grid)
if best is quad_model:
    Z = (best.params["const"]
         + best.params["孕周数"]*WW
         + best.params["BMI"]*BB
         + best.params["孕周数2"]*(WW**2)
         + best.params["BMI2"]*(BB**2)
         + best.params["交互"]*(WW*BB))
else:
    Z = (best.params["const"]
         + best.params["孕周数"]*WW
         + best.params["BMI"]*BB)

plt.figure(figsize=(9,7))
cs = plt.contourf(WW, BB, Z, levels=18)
cbar = plt.colorbar(cs)
cbar.set_label("预测Y染色体浓度")
# 绘制4%阈值曲线并添加标签（修正格式问题）
threshold = plt.contour(WW, BB, Z, levels=[0.04], linewidths=1.2)
plt.clabel(threshold, inline=True, fmt=lambda x: f'4%阈值', fontsize=12)  # 使用lambda函数避免格式化问题

plt.title(f"图4 模型（{best_name}）预测等高线：孕周×BMI→Y染色体浓度")
plt.xlabel("孕周数（周）")
plt.ylabel("BMI")
plt.tight_layout()
plt.show()

# 女胎表可能没有 Y染色体浓度列
if "Y染色体浓度" in female.columns:
    f = female[["孕周数","BMI","Y染色体浓度"]].dropna()
    if len(f) > 0:
        check_stats = f["Y染色体浓度"].describe(percentiles=[0.5,0.9,0.99]).to_frame(name="女胎Y浓度分布")
        print(check_stats)
else:
    print("女胎表无 Y染色体浓度 列，符合预期（女胎无Y染色体）。")

# —— 推荐时点：按BMI四分位，求达到4%的最早孕周（基于最优模型）——
def earliest_week_for_threshold(bmi, y_thr=0.04):
    # 在可行范围内搜索周数
    ws = np.linspace(m["孕周数"].min(), m["孕周数"].max(), 500)
    if best is quad_model:
        yhat = (best.params["const"]
                + best.params["孕周数"]*ws
                + best.params["BMI"]*bmi
                + best.params["孕周数2"]*(ws**2)
                + best.params["BMI2"]*(bmi**2)
                + best.params["交互"]*(ws*bmi))
    else:
        yhat = (best.params["const"]
                + best.params["孕周数"]*ws
                + best.params["BMI"]*bmi)
    ws_ok = ws[yhat >= y_thr]
    return ws_ok.min() if ws_ok.size>0 else np.nan

bmi_quants = m["BMI"].quantile([0.1,0.25,0.5,0.75,0.9]).round(2)
rows = []
for label, bmi_v in bmi_quants.items():
    w_star = earliest_week_for_threshold(bmi_v)
    rows.append({"BMI分位":label, "BMI值":bmi_v, "预测首达4%孕周": None if pd.isna(w_star) else round(float(w_star),2)})
rec_table = pd.DataFrame(rows)
print("按BMI分位的首达4%孕周（模型推断）:\n", rec_table)