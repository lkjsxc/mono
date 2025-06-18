document.addEventListener('DOMContentLoaded', function() {
    const form = document.getElementById('eventForm');
    const result = document.getElementById('result');
    
    form.addEventListener('submit', async function(e) {
        e.preventDefault();
        
        const eventId = document.getElementById('eventId').value;
        const content = document.getElementById('content').value;
        
        const event = {
            event_id: eventId,
            content: content
        };
        
        try {
            const response = await fetch('/', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify([event])
            });
            
            const responseData = await response.text();
            
            if (response.ok) {
                result.className = 'success';
                result.textContent = 'Event submitted successfully! Response: ' + responseData;
                form.reset();
            } else {
                result.className = 'error';
                result.textContent = 'Error submitting event: ' + response.status + ' ' + response.statusText;
            }
        } catch (error) {
            result.className = 'error';
            result.textContent = 'Network error: ' + error.message;
        }
        
        result.style.display = 'block';
    });
});
