#version 120

varying vec3 fragPos;
varying vec3 normal_v;
varying vec2 TexCoord;

uniform sampler2D diffuse_tex;
uniform sampler2D normal_tex;
uniform vec3 light_pos_v;
uniform vec3 view_pos;

// Базовые цвета шерсти (многослойные для объёма)
const vec3 FUR_BASE = vec3(0.82, 0.72, 0.62);      // Светлый подшёрсток
const vec3 FUR_MID = vec3(0.68, 0.56, 0.45);       // Основная шерсть
const vec3 FUR_SHADOW = vec3(0.42, 0.34, 0.28);    // Глубокие тени
const vec3 FUR_RIM = vec3(0.92, 0.85, 0.78);       // Контровой свет на краях (эффект пушистости)

// имитация текстуры шерсти
float furNoise(vec2 uv, vec3 pos) {
    return fract(sin(dot(uv + pos.xy, vec2(12.9898, 78.233))) * 43758.5453 + pos.z * 0.1);
}

void main()
{
    // Базовый цвет из текстуры
    vec3 tex_color = texture2D(diffuse_tex, TexCoord).rgb;
    
    // Если текстура чёрная/пустая — берём базовый цвет шерсти
    if (tex_color.r < 0.15 && tex_color.g < 0.15 && tex_color.b < 0.15) {
        tex_color = FUR_BASE;
    }
    
    // Нормаль с защитой от артефактов
    vec3 N = length(normal_v) < 0.001 ? vec3(0.0, 1.0, 0.0) : normalize(normal_v);
    
    // Вектора освещения
    vec3 L = normalize(light_pos_v - fragPos);
    vec3 V = normalize(view_pos - fragPos);
    float NdotV = max(dot(N, V), 0.0);
    
    // Пушистость
    float rim = pow(1.0 - NdotV, 2.5);
    vec3 rim_light = rim * FUR_RIM * 0.35;
    
    // Мягкое основное освещение
    float diff = max(dot(N, L), 0.0);
    
    // Многослойная шерсть
    vec3 fur_color;
    if (diff > 0.7) {
        fur_color = mix(FUR_MID, FUR_BASE, 0.6);  // Яркие участки — светлый подшёрсток
    } else if (diff > 0.3) {
        fur_color = FUR_MID;                        // Средняя освещённость — основной тон
    } else {
        fur_color = mix(FUR_MID, FUR_SHADOW, 0.5);  // Тени — глубокий тон
    }
    
    // Имитация отдельных ворсинок
    float noise = furNoise(TexCoord, fragPos) * 0.04 - 0.02; // ±2% яркости
    fur_color += vec3(noise);
    
    // Мягкий, широкий блик
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), 18.0) * 0.08;
    vec3 specular = spec * vec3(0.7, 0.65, 0.6);
    
    // Финальный цвет
    vec3 ambient = vec3(0.22) * fur_color;  // Мягкий фоновый свет
    vec3 diffuse = diff * fur_color;
    
    vec3 final_color = ambient + diffuse + specular + rim_light;
    
    // "Теплота" в тенях
    if (diff < 0.25) {
        final_color = mix(final_color, final_color * vec3(1.05, 0.98, 0.92), 0.3);
    }
    
    gl_FragColor = vec4(final_color, 1.0);
}