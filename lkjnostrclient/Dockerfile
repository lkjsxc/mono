# Use nginx alpine image for a lightweight web server
FROM nginx:alpine

# Copy HTML files and other content from route directory to nginx web root
COPY route/ /usr/share/nginx/html/

# Copy custom nginx configuration if needed
COPY nginx.conf /etc/nginx/nginx.conf

# Expose port 80
EXPOSE 80

# Start nginx
CMD ["nginx", "-g", "daemon off;"]